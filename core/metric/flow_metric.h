#pragma once

#include <atomic>
#include <utils/fixed_string.h>

template <FixedString File, FixedString Function, size_t Line>
struct FlowMetric
{
    static constexpr int BUCKET_COUNT = 32;
    std::atomic<uint64_t> buckets[BUCKET_COUNT]{};

    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> bytes{0};
    std::atomic<uint64_t> total_delay_ns{0};
    std::atomic<uint64_t> max_delay_ns{0};

    std::string_view file = trim_project_path(File);
    std::string_view function = Function;
    size_t line = Line;

    static inline int bucket_index(uint64_t ns)
    {
        if (ns == 0) return 0;
        int idx = 64 - __builtin_clzll(ns);
        return std::min(idx, BUCKET_COUNT - 1);
    }

    inline void record(uint64_t delay_ns, uint64_t byte_count = 0)
    {
        count.fetch_add(1, std::memory_order_relaxed);
        bytes.fetch_add(byte_count, std::memory_order_relaxed);
        total_delay_ns.fetch_add(delay_ns, std::memory_order_relaxed);

        uint64_t old_max = max_delay_ns.load(std::memory_order_relaxed);
        while (delay_ns > old_max && !max_delay_ns.compare_exchange_weak(
                old_max, delay_ns,
                std::memory_order_relaxed,
                std::memory_order_relaxed))
        {}

        buckets[bucket_index(delay_ns)].fetch_add(1, std::memory_order_relaxed);
    }

    inline uint64_t percentile(double p) const
    {
        uint64_t total = count.load(std::memory_order_relaxed);
        if (total == 0) return 0;

        uint64_t target = static_cast<uint64_t>(std::ceil(total * p));
        uint64_t cumulative = 0;

        for (int i = 0; i < BUCKET_COUNT; ++i)
        {
            cumulative += buckets[i].load(std::memory_order_relaxed);
            if (cumulative >= target)
            {
                return bucket_upper_bound_ns(i);
            }
        }

        return max_delay_ns.load(std::memory_order_relaxed);
    }

    static inline uint64_t bucket_upper_bound_ns(int idx)
    {
        if (idx <= 0) return 1;
        if (idx >= 63) return UINT64_MAX;
        return 1ULL << idx;
    }

    static inline std::string_view trim_project_path(std::string_view path)
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