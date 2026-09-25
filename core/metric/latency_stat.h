#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include <utils/fixed_string.h>

struct LatencyStats
{
    // 8 buckets per power-of-two range.
    //
    // Examples:
    //   0..15 ns   -> exact 1 ns buckets
    //   16..31 ns  -> 2 ns buckets
    //   32..63 ns  -> 4 ns buckets
    //   64..127 ns -> 8 ns buckets
    //
    // This gives at most ~12.5% bucket-width error while still keeping
    // record() O(1), allocation-free, and lock-free at the application level.
    static constexpr int SUB_BUCKET_BITS = 3;
    static constexpr int SUB_BUCKET_COUNT = 1 << SUB_BUCKET_BITS; // 8

    // Bucket 0 stores exactly 0.
    // Buckets 1..7 store exactly 1..7.
    // From bucket 8 onward, each power-of-two range is split into 8 buckets.
    //
    // Exponents 3..63 inclusive => 61 ranges.
    static constexpr int FIRST_LOG_BUCKET = SUB_BUCKET_COUNT; // 8;
    static constexpr int LOG_RANGE_COUNT = 64 - SUB_BUCKET_BITS; // 61
    static constexpr int BUCKET_COUNT =
        FIRST_LOG_BUCKET + LOG_RANGE_COUNT * SUB_BUCKET_COUNT; // 496

    std::atomic<uint64_t> buckets[BUCKET_COUNT]{};

    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> total_delay_ns{0};
    std::atomic<uint64_t> max_delay_ns{0};

    static inline int bucket_index(uint64_t ns) noexcept
    {
        // Keep very small values exact.
        if (ns < static_cast<uint64_t>(FIRST_LOG_BUCKET))
        {
            return static_cast<int>(ns);
        }

        // exponent = floor(log2(ns)).
        // ns >= 8 here, so __builtin_clzll(0) is never possible.
        const int exponent = 63 - __builtin_clzll(ns);

        // Split [2^exponent, 2^(exponent + 1)) into 8 equal buckets.
        //
        // Example for exponent = 10:
        //   range = [1024, 2048)
        //   bucket width = 128
        //   sub-buckets:
        //     [1024, 1152), [1152, 1280), ... [1920, 2048)
        const int shift = exponent - SUB_BUCKET_BITS;
        const int sub_bucket =
            static_cast<int>((ns >> shift) & (SUB_BUCKET_COUNT - 1));

        return FIRST_LOG_BUCKET
             + (exponent - SUB_BUCKET_BITS) * SUB_BUCKET_COUNT
             + sub_bucket;
    }

    static constexpr uint64_t bucket_upper_bound_ns(int idx) noexcept
    {
        if (idx <= 0)
        {
            return 0;
        }

        // Buckets 1..7 are exact.
        if (idx < FIRST_LOG_BUCKET)
        {
            return static_cast<uint64_t>(idx);
        }

        const int offset = idx - FIRST_LOG_BUCKET;
        const int exponent =
            SUB_BUCKET_BITS + offset / SUB_BUCKET_COUNT;
        const int sub_bucket =
            offset % SUB_BUCKET_COUNT;

        const int shift = exponent - SUB_BUCKET_BITS;
        const uint64_t step = 1ULL << shift;
        const uint64_t lower =
            (1ULL << exponent)
            + (static_cast<uint64_t>(sub_bucket) << shift);

        // Because lower is aligned to step, OR-ing with step - 1 gives
        // the inclusive upper bound without overflowing at UINT64_MAX.
        return lower | (step - 1);
    }

    inline void record(uint64_t delay_ns) noexcept
    {
        // Update histogram first. percentile() snapshots only the histogram,
        // so its total and cumulative counts always come from the same data.
        buckets[bucket_index(delay_ns)].fetch_add(
            1, std::memory_order_relaxed);

        total_delay_ns.fetch_add(
            delay_ns, std::memory_order_relaxed);

        uint64_t old_max =
            max_delay_ns.load(std::memory_order_relaxed);

        while (delay_ns > old_max &&
               !max_delay_ns.compare_exchange_weak(
                   old_max,
                   delay_ns,
                   std::memory_order_relaxed,
                   std::memory_order_relaxed))
        {
        }

        count.fetch_add(1, std::memory_order_relaxed);
    }

    inline uint64_t percentile(double p) const noexcept
    {
        if (!(p > 0.0 && p <= 1.0))
        {
            return 0;
        }

        // Capture one self-consistent histogram snapshot for this calculation.
        //
        // It is intentionally not a globally atomic "point in time" snapshot:
        // writers remain completely lock-free. However, target and cumulative
        // are computed from the exact same captured bucket values, so there is
        // no count-vs-bucket mismatch and no bogus fallback to max_delay_ns.
        std::array<uint64_t, BUCKET_COUNT> snapshot{};
        uint64_t total = 0;

        for (int i = 0; i < BUCKET_COUNT; ++i)
        {
            snapshot[i] =
                buckets[i].load(std::memory_order_relaxed);
            total += snapshot[i];
        }

        if (total == 0)
        {
            return 0;
        }

        // Nearest-rank percentile.
        const uint64_t target = static_cast<uint64_t>(
            std::ceil(static_cast<long double>(total)
                      * static_cast<long double>(p)));

        uint64_t cumulative = 0;

        for (int i = 0; i < BUCKET_COUNT; ++i)
        {
            cumulative += snapshot[i];

            if (cumulative >= target)
            {
                return bucket_upper_bound_ns(i);
            }
        }

        // Unreachable as long as total was computed from this snapshot.
        return bucket_upper_bound_ns(BUCKET_COUNT - 1);
    }
};

static_assert(
    LatencyStats::bucket_upper_bound_ns(
        LatencyStats::BUCKET_COUNT - 1)
        == std::numeric_limits<uint64_t>::max(),
    "LatencyStats histogram must cover the full uint64_t range");

