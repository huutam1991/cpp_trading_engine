#pragma once

#include <atomic>
#include <utils/fixed_string.h>

template <FixedString File, FixedString Function, size_t Line>
struct FlowMetric
{
    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> bytes{0};
    std::atomic<uint64_t> total_delay_ns{0};
    std::atomic<uint64_t> max_delay_ns{0};

    std::string_view file = trim_project_path(File);
    std::string_view function = Function;
    size_t line = Line;

    static std::string_view trim_project_path(std::string_view path)
    {
        constexpr std::string_view marker = "cpp_trading_engine/";

        auto pos = path.find(marker);
        if (pos != std::string_view::npos)
        {
            return path.substr(pos + marker.size());
        }

        return path;
    }
};