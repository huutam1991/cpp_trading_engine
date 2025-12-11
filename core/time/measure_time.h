#pragma once

#include <chrono>
#include <x86intrin.h>
#include <spdlog/spdlog.h>

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

class MeasurePipeLineTime
{
    static inline uint64_t rdtscp()
    {
        unsigned aux;
        return __rdtscp(&aux);
    }

    static inline double cycles_to_us(uint64_t cycles, double cpu_ghz)
    {
        return cycles / (cpu_ghz * 1.0);
    }
};