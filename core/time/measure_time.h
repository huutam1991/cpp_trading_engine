#pragma once

#include <chrono>
#include <x86intrin.h>
#include <spdlog/spdlog.h>
#include <cstdint>

#include <enum_reflect/enum_reflect.h>
#include <utils/util_macros.h>

enum class PipelineStage
{
    RECEIVE_DATA,
    PARSE_DATA,
    ORDER_BOOK_UPDATE,
    PRICING_UPDATE,
    RISK_UPDATE,
    STRATEGY_UPDATE,
    SEND_ORDER,
    TOTAL_STAGES
};

struct ScopeTiming
{
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t ticks = 0;
    double ns = 0.0;
    double us = 0.0;
    PipelineStage stage = PipelineStage::TOTAL_STAGES;
};

class MeasureTime
{
public:
    static void init()
    {
        get_tsc_ghz();
    }

    static double get_tsc_ghz()
    {
        static double tsc_ghz = calibrate_tsc_ghz(500);
        return tsc_ghz;
    }

    explicit MeasureTime(const std::string& logs) : m_logs(logs)
    {
        _mm_lfence();
        m_result.start = __rdtsc();
    }

    // Read TSC with an optional fence to prevent instruction reordering
    static inline uint64_t read_tsc()
    {
        // __cpuid or _mm_lfence can serialize the pipeline
        _mm_lfence();
        uint64_t tsc = __rdtsc();
        return tsc;
    }

    ~MeasureTime()
    {
        unsigned aux;

        const uint64_t end = __rdtscp(&aux);
        _mm_lfence();

        m_result.ticks = end - m_result.start;
        m_result.ns = static_cast<double>(m_result.ticks) / get_tsc_ghz();
        m_result.us = m_result.ns / 1000.0;

        spdlog::debug("Execute time - {}: {} ticks, {} ns, {} us", m_logs, m_result.ticks, m_result.ns, m_result.us);
    }

    MeasureTime(const MeasureTime&) = delete;
    MeasureTime& operator=(const MeasureTime&) = delete;

private:
    std::string m_logs;
    ScopeTiming m_result;

    static double calibrate_tsc_ghz(int ms_wait = 100)
    {
        // Optional: pin thread to a single core for consistent measurements
        // (Platform-specific code like pthread_setaffinity_np can go here)

        uint64_t tsc_start = read_tsc();
        auto wall_start = std::chrono::high_resolution_clock::now();

        // Sleep for the calibration window
        std::this_thread::sleep_for(std::chrono::milliseconds(ms_wait));

        uint64_t tsc_end = read_tsc();
        auto wall_end = std::chrono::high_resolution_clock::now();

        // Calculate elapsed time and cycles
        std::chrono::duration<double, std::milli> elapsed_ms = wall_end - wall_start;
        uint64_t elapsed_ticks = tsc_end - tsc_start;

        // GHz = ticks per nanosecond / cycles per second vs nanoseconds
        double ghz = (double)elapsed_ticks / (elapsed_ms.count() * 1e6);

        return ghz;
    }
};

using TraceId = uint32_t;

class PipelineTraceBuffer
{
public:
    static constexpr TraceId Capacity = 20000;

    static inline TraceId allocate() noexcept
    {
        const TraceId id = m_next++;

        if (m_next == Capacity)
        {
            m_next = 0;
        }

        return id;
    }

    template <PipelineStage Name>
    static inline ScopeTiming& get(TraceId id) noexcept
    {
        return field<Name>[id];
    }

    template <PipelineStage StartStage, PipelineStage EndStage>
    static inline ScopeTiming get_pipeline_timing(TraceId id, bool force_end = true)
    {
        ScopeTiming timing;
        timing.start = field<StartStage>[id].start;
        timing.end = force_end ? MeasureTime::read_tsc() : field<EndStage>[id].end;
        timing.ticks = timing.end - timing.start;
        timing.ns = static_cast<double>(timing.ticks) / MeasureTime::get_tsc_ghz();
        timing.us = timing.ns / 1000.0;
        return timing;
    }

    template <PipelineStage Name>
    class RecordStageTiming
    {
    public:
        inline RecordStageTiming(TraceId id, bool end = false) : m_timing{field<Name>[id]}, m_id{id}, m_end{end}
        {
            m_timing.start = MeasureTime::read_tsc();
        }

        inline ~RecordStageTiming()
        {
            m_timing.end = MeasureTime::read_tsc();
            m_timing.ticks = m_timing.end - m_timing.start;
            m_timing.ns = static_cast<double>(m_timing.ticks) / MeasureTime::get_tsc_ghz();
            m_timing.us = m_timing.ns / 1000.0;

            if (m_end) [[unlikely]]
            {
                update_pipeline_timing(m_id);
            }
        }

    private:
        ScopeTiming& m_timing;
        TraceId m_id;
        bool m_end;

        template <typename F, std::size_t... I>
        static constexpr void for_each_stage_impl(
            F&& f,
            std::index_sequence<I...>)
        {
            (
                f.template operator()<
                    static_cast<PipelineStage>(I)
                >(),
                ...
            );
        }

        template <typename F>
        static constexpr void for_each_stage(F&& f)
        {
            for_each_stage_impl(
                std::forward<F>(f),
                std::make_index_sequence<
                    static_cast<std::size_t>(
                        PipelineStage::TOTAL_STAGES)
                >{}
            );
        }

        void update_pipeline_timing(TraceId id)
        {
            std::array<ScopeTiming, static_cast<size_t>(PipelineStage::TOTAL_STAGES)> pipeline_timings;
            size_t count = 0;

            for_each_stage([&]<PipelineStage Stage>()
            {
                ScopeTiming& scope = field<Stage>[id];
                if (scope.start == 0 || scope.end == 0)
                {
                    return;
                }

                scope.stage = Stage;
                pipeline_timings[count++] = scope;
            });


            std::sort(pipeline_timings.begin(), pipeline_timings.begin() + count, [](const ScopeTiming& a, const ScopeTiming& b)
            {
                return a.start < b.start && a.end < b.end;
            });

            // Update the pipeline timing for the current stage
            m_timing.start = pipeline_timings[0].start;
            m_timing.end = pipeline_timings[count - 1].end;
            m_timing.ticks = m_timing.end - m_timing.start;
            m_timing.ns = static_cast<double>(m_timing.ticks) / MeasureTime::get_tsc_ghz();
            m_timing.us = m_timing.ns / 1000.0;

            spdlog::debug("Pipeline timing for TraceId {}", id);
            for (size_t i = 0; i < count; ++i)
            {
                auto& timing = pipeline_timings[i];
                spdlog::debug("---- Stage: {}, Start: {}, End: {}, Ticks: {}, ns: {}, us: {}",
                    enum_reflect::enum_name(timing.stage), timing.start, timing.end, timing.ticks, timing.ns, timing.us);
            }

            spdlog::debug("---- Total Pipeline Timing: Start: {}, End: {}, Ticks: {}, ns: {}, us: {}",
                m_timing.start, m_timing.end, m_timing.ticks, m_timing.ns, m_timing.us);
        }
    };

private:
    static inline TraceId m_next;

    template <PipelineStage Stage>
    static inline std::array<ScopeTiming, Capacity> field;
};