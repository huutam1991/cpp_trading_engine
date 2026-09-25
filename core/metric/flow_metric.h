#pragma once

#include <metric/latency_stat.h>

template <FixedString File, FixedString Function, size_t Line>
struct FlowMetric : LatencyStats
{
    std::atomic<uint64_t> bytes{0};

    std::string_view file = trim_project_path(File);
    std::string_view function = Function;

    size_t line = Line;

    inline void record(
        uint64_t delay_ns,
        uint64_t byte_count = 0) noexcept
    {
        LatencyStats::record(delay_ns);
        bytes.fetch_add(byte_count, std::memory_order_relaxed);
    }

    static inline std::string_view trim_project_path(
        std::string_view path) noexcept
    {
        constexpr std::string_view marker = "cpp_trading_engine/";

        const auto pos = path.find(marker);

        if (pos != std::string_view::npos)
        {
            return path.substr(pos + marker.size());
        }

        return path;
    }
};