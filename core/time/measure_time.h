#pragma once

#include <chrono>
#include <x86intrin.h>
#include <spdlog/spdlog.h>
#include <cstdint>

#include <enum_reflect/enum_reflect.h>
#include <utils/util_macros.h>

enum MeasureUnit
{
    NANOSECOND = 0,
    MICROSECOND,
    MILLISECOND,
    SECOND
};

class MeasureTime
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    std::string m_logs;
    MeasureUnit m_measure_unit;
    bool m_is_stop = false;

public:
    MeasureTime(std::string logs, MeasureUnit measure_unit = MeasureUnit::NANOSECOND) : m_logs{logs}, m_measure_unit{measure_unit}
    {
        start = std::chrono::high_resolution_clock::now();
    }

    ~MeasureTime()
    {
        end = m_is_stop == false ? std::chrono::high_resolution_clock::now() : end;
        auto duration_count = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        double execute_time = (double)duration_count / std::pow(1000.0, m_measure_unit);
        std::string_view unit = enum_reflect::enum_name(m_measure_unit);

        spdlog::debug("Execute time - {}: {} {}", m_logs, execute_time, unit);
    }

    void stop_counting()
    {
        end = std::chrono::high_resolution_clock::now();
        m_is_stop = true;
    }
};

struct PipelineTiming
{
    uint64_t ticks{};
    double ns{};
    double us{};
};

class MeasurePipelineTime
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

    explicit MeasurePipelineTime(double tsc_ghz, PipelineTiming& result)
        : tsc_ghz_(tsc_ghz), result_(result)
    {
        _mm_lfence();
        start_ = __rdtsc();
    }

    ~MeasurePipelineTime()
    {
        unsigned aux;

        const uint64_t end = __rdtscp(&aux);
        _mm_lfence();

        result_.ticks = end - start_;
        result_.ns = static_cast<double>(result_.ticks) / tsc_ghz_;
        result_.us = result_.ns / 1000.0;

        spdlog::debug("Pipeline timing: {} ticks, {} ns, {} us", result_.ticks, result_.ns, result_.us);
    }

    MeasurePipelineTime(const MeasurePipelineTime&) = delete;
    MeasurePipelineTime& operator=(const MeasurePipelineTime&) = delete;

private:
    uint64_t start_{};

    double tsc_ghz_;
    PipelineTiming& result_;

    // Read TSC with an optional fence to prevent instruction reordering
    static inline uint64_t read_tsc()
    {
        // __cpuid or _mm_lfence can serialize the pipeline
        _mm_lfence();
        uint64_t tsc = __rdtsc();
        return tsc;
    }

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