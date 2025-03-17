#ifndef MEASURE_TIME
#define MEASURE_TIME

#include <chrono>
#include <util_macros.h>

enum MeasureUnit
{
    SECOND,
    MILLISECOND,
    MICROSECOND,
    NANOSECOND
};

class MeasureTime
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::string m_logs;
    MeasureUnit m_measure_unit;

public:
    MeasureTime(std::string logs, MeasureUnit measure_unit = MeasureUnit::NANOSECOND) : m_logs{logs}, m_measure_unit{measure_unit}
    {
        start = std::chrono::high_resolution_clock::now();
    }

    ~MeasureTime()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_count = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        double execute_time;
        std::string unit;

        switch (m_measure_unit)
        {
        case MeasureUnit::SECOND:
            execute_time = (double)duration_count / 1000000000.0;
            unit = "seconds";
            break;
        case MeasureUnit::MILLISECOND:
            execute_time = (double)duration_count / 1000000.0;
            unit = "milliseconds";
            break;
        case MeasureUnit::MICROSECOND:
            execute_time = (double)duration_count / 1000.0;
            unit = "microseconds";
            break;
        case MeasureUnit::NANOSECOND:
            execute_time = (double)duration_count / 1.0;
            unit = "nanoseconds";
            break;

        // Default is millisecond
        default:
            execute_time = (double)duration_count / 1000000.0;
            unit = "milliseconds";
            break;
        }

        ADD_LOG("Execute time - " << m_logs << ": " << execute_time << " " << unit);
    }
};

#endif //MEASURE_TIME