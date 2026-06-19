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

    std::string_view file = File;
    std::string_view function = Function;
    size_t line = Line;
};