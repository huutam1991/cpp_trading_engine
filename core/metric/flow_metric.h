#pragma once

#include <atomic>

struct FlowMetric
{
    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> bytes{0};
    std::atomic<uint64_t> total_delay_ns{0};
    std::atomic<uint64_t> max_delay_ns{0};

    const char* file = "";
    const char* function = "";
    uint32_t line = 0;
};