#pragma once

#include <chrono>
#include <x86intrin.h>
#include <spdlog/spdlog.h>
#include <cstdint>

#include <enum_reflect/enum_reflect.h>
#include <utils/util_macros.h>
#include <utils/fixed_string.h>

struct ScopeTiming
{
    uint64_t start{};
    uint64_t end{};
    uint64_t ticks{};
    double ns{};
    double us{};
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

    inline TraceId allocate() noexcept
    {
        const TraceId id = m_next++;

        if (m_next == Capacity)
        {
            m_next = 0;
        }

        return id;
    }

    template <FixedString Name>
    inline ScopeTiming& get(TraceId id) noexcept
    {
        return field<Name>[id];
    }

    template <FixedString StartStage, FixedString EndStage>
    inline ScopeTiming get_pipeline_timing(TraceId id)
    {
        ScopeTiming timing;
        timing.start = field<StartStage>[id].start;
        timing.end = field<EndStage>[id].end;
        timing.ticks = timing.end - timing.start;
        timing.ns = static_cast<double>(timing.ticks) / MeasureTime::get_tsc_ghz();
        timing.us = timing.ns / 1000.0;
        return timing;
    }

    template <FixedString Name>
    class RecordStageTiming
    {
    public:
        inline RecordStageTiming(TraceId id) : m_timing{field<Name>[id]}
        {
            m_timing.start = MeasureTime::read_tsc();
        }

        inline ~RecordStageTiming()
        {
            m_timing.end = MeasureTime::read_tsc();
            m_timing.ticks = m_timing.end - m_timing.start;
            m_timing.ns = static_cast<double>(m_timing.ticks) / MeasureTime::get_tsc_ghz();
            m_timing.us = m_timing.ns / 1000.0;
        }

    private:
        ScopeTiming& m_timing;
    };

private:
    TraceId m_next{};

    template <FixedString Name>
    static inline std::array<ScopeTiming, Capacity> field;
};

extern PipelineTraceBuffer g_pipeline_trace_buffer;