#include <gtest/gtest.h>
#include <metric/latency_stat.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <thread>
#include <vector>

namespace
{

uint64_t reported_value(uint64_t ns)
{
    const int idx = LatencyStats::bucket_index(ns);
    return LatencyStats::bucket_upper_bound_ns(idx);
}

}


// ============================================================================
// bucket_index / bucket_upper_bound
// ============================================================================

TEST(LatencyStatsTest, ZeroBucket)
{
    EXPECT_EQ(LatencyStats::bucket_index(0), 0);
    EXPECT_EQ(LatencyStats::bucket_upper_bound_ns(0), 0);
}


TEST(LatencyStatsTest, SmallValuesAreExact)
{
    // 0..15 ns should have exact 1 ns resolution.
    for (uint64_t ns = 0; ns <= 15; ++ns)
    {
        const int idx = LatencyStats::bucket_index(ns);

        EXPECT_EQ(
            LatencyStats::bucket_upper_bound_ns(idx),
            ns
        ) << "ns = " << ns;
    }
}


TEST(LatencyStatsTest, FirstNonExactBuckets)
{
    // 16..31 ns: width = 2 ns.
    EXPECT_EQ(reported_value(16), 17);
    EXPECT_EQ(reported_value(17), 17);

    EXPECT_EQ(reported_value(18), 19);
    EXPECT_EQ(reported_value(19), 19);

    EXPECT_EQ(reported_value(30), 31);
    EXPECT_EQ(reported_value(31), 31);
}


TEST(LatencyStatsTest, PowerOfTwoBoundary)
{
    // Last bucket below 1024.
    EXPECT_EQ(reported_value(1023), 1023);

    // [1024, 1151]
    EXPECT_EQ(reported_value(1024), 1151);
    EXPECT_EQ(reported_value(1100), 1151);
    EXPECT_EQ(reported_value(1151), 1151);

    // Next bucket [1152, 1279].
    EXPECT_EQ(reported_value(1152), 1279);
}


TEST(LatencyStatsTest, BoundaryImmediatelyBeforeAndAfterPowerOfTwo)
{
    EXPECT_EQ(reported_value(511), 511);

    // 512 range has step = 64.
    EXPECT_EQ(reported_value(512), 575);

    EXPECT_EQ(reported_value(1023), 1023);
    EXPECT_EQ(reported_value(1024), 1151);

    EXPECT_EQ(reported_value(2047), 2047);
    EXPECT_EQ(reported_value(2048), 2303);
}


TEST(LatencyStatsTest, MaximumUint64Value)
{
    constexpr uint64_t MAX =
        std::numeric_limits<uint64_t>::max();

    EXPECT_EQ(
        LatencyStats::bucket_index(MAX),
        LatencyStats::BUCKET_COUNT - 1
    );

    EXPECT_EQ(
        LatencyStats::bucket_upper_bound_ns(
            LatencyStats::BUCKET_COUNT - 1),
        MAX
    );

    EXPECT_EQ(reported_value(MAX), MAX);
}


TEST(LatencyStatsTest, HighestPowerOfTwo)
{
    constexpr uint64_t value = 1ULL << 63;

    const int idx = LatencyStats::bucket_index(value);

    EXPECT_EQ(
        LatencyStats::bucket_upper_bound_ns(idx),
        0x8FFFFFFFFFFFFFFFULL
    );
}


/*
 * This is the most important bucket test.
 *
 * It verifies EVERY bucket:
 *
 *   previous upper + 1 == next lower
 *
 * and verifies that both lower and upper values map back to exactly
 * the expected bucket.
 *
 * This catches:
 *   - gaps
 *   - overlapping buckets
 *   - off-by-one errors
 *   - wrong power-of-two transitions
 *   - wrong final bucket
 *   - uint64 boundary errors
 */
TEST(LatencyStatsTest, AllBucketBoundariesAreContiguous)
{
    uint64_t previous_upper = 0;

    for (int i = 0; i < LatencyStats::BUCKET_COUNT; ++i)
    {
        const uint64_t upper =
            LatencyStats::bucket_upper_bound_ns(i);

        const uint64_t lower =
            (i == 0)
                ? 0
                : previous_upper + 1;

        EXPECT_EQ(
            LatencyStats::bucket_index(lower),
            i
        ) << "bucket = " << i
          << ", lower = " << lower;

        EXPECT_EQ(
            LatencyStats::bucket_index(upper),
            i
        ) << "bucket = " << i
          << ", upper = " << upper;

        if (i > 0)
        {
            EXPECT_GT(
                upper,
                previous_upper
            ) << "bucket = " << i;
        }

        previous_upper = upper;
    }

    EXPECT_EQ(
        previous_upper,
        std::numeric_limits<uint64_t>::max()
    );
}


// ============================================================================
// record()
// ============================================================================

TEST(LatencyStatsTest, InitialState)
{
    LatencyStats stats;

    EXPECT_EQ(stats.count.load(), 0);
    EXPECT_EQ(stats.total_delay_ns.load(), 0);
    EXPECT_EQ(stats.max_delay_ns.load(), 0);

    for (int i = 0; i < LatencyStats::BUCKET_COUNT; ++i)
    {
        EXPECT_EQ(stats.buckets[i].load(), 0);
    }
}


TEST(LatencyStatsTest, RecordSingleValue)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(stats.count.load(), 1);
    EXPECT_EQ(stats.total_delay_ns.load(), 10);
    EXPECT_EQ(stats.max_delay_ns.load(), 10);

    EXPECT_EQ(
        stats.buckets[
            LatencyStats::bucket_index(10)
        ].load(),
        1
    );
}


TEST(LatencyStatsTest, RecordMultipleValues)
{
    LatencyStats stats;

    stats.record(10);
    stats.record(20);
    stats.record(5);

    EXPECT_EQ(stats.count.load(), 3);
    EXPECT_EQ(stats.total_delay_ns.load(), 35);
    EXPECT_EQ(stats.max_delay_ns.load(), 20);
}


TEST(LatencyStatsTest, MaximumDoesNotDecrease)
{
    LatencyStats stats;

    stats.record(1000);
    EXPECT_EQ(stats.max_delay_ns.load(), 1000);

    stats.record(500);
    EXPECT_EQ(stats.max_delay_ns.load(), 1000);

    stats.record(999);
    EXPECT_EQ(stats.max_delay_ns.load(), 1000);

    stats.record(1001);
    EXPECT_EQ(stats.max_delay_ns.load(), 1001);
}


TEST(LatencyStatsTest, RecordZero)
{
    LatencyStats stats;

    stats.record(0);

    EXPECT_EQ(stats.count.load(), 1);
    EXPECT_EQ(stats.total_delay_ns.load(), 0);
    EXPECT_EQ(stats.max_delay_ns.load(), 0);

    EXPECT_EQ(stats.buckets[0].load(), 1);
}


TEST(LatencyStatsTest, RecordRepeatedValue)
{
    LatencyStats stats;

    constexpr int N = 1000;

    for (int i = 0; i < N; ++i)
    {
        stats.record(42);
    }

    EXPECT_EQ(stats.count.load(), N);
    EXPECT_EQ(stats.total_delay_ns.load(), 42ULL * N);
    EXPECT_EQ(stats.max_delay_ns.load(), 42);

    const int idx = LatencyStats::bucket_index(42);

    EXPECT_EQ(stats.buckets[idx].load(), N);
}


// ============================================================================
// percentile()
// ============================================================================

TEST(LatencyStatsTest, PercentileEmptyStats)
{
    LatencyStats stats;

    EXPECT_EQ(stats.percentile(0.50), 0);
    EXPECT_EQ(stats.percentile(0.90), 0);
    EXPECT_EQ(stats.percentile(0.99), 0);
    EXPECT_EQ(stats.percentile(1.00), 0);
}


TEST(LatencyStatsTest, PercentileSingleExactValue)
{
    LatencyStats stats;

    stats.record(7);

    EXPECT_EQ(stats.percentile(0.01), 7);
    EXPECT_EQ(stats.percentile(0.50), 7);
    EXPECT_EQ(stats.percentile(0.90), 7);
    EXPECT_EQ(stats.percentile(0.99), 7);
    EXPECT_EQ(stats.percentile(1.00), 7);
}


TEST(LatencyStatsTest, PercentileSingleNonExactValue)
{
    LatencyStats stats;

    stats.record(934);

    /*
     * 934 belongs to:
     *
     *   [896, 959]
     *
     * therefore percentile returns inclusive upper bound = 959.
     */
    EXPECT_EQ(stats.percentile(0.50), 959);
    EXPECT_EQ(stats.percentile(0.99), 959);
    EXPECT_EQ(stats.percentile(1.00), 959);
}


TEST(LatencyStatsTest, PercentileAllZeros)
{
    LatencyStats stats;

    for (int i = 0; i < 100; ++i)
    {
        stats.record(0);
    }

    EXPECT_EQ(stats.count.load(), 100);

    EXPECT_EQ(stats.percentile(0.50), 0);
    EXPECT_EQ(stats.percentile(0.90), 0);
    EXPECT_EQ(stats.percentile(0.99), 0);
    EXPECT_EQ(stats.percentile(1.00), 0);
}


TEST(LatencyStatsTest, NearestRankPercentiles)
{
    LatencyStats stats;

    /*
     * Keep everything <= 15 so histogram values are exact.
     *
     * Samples:
     *
     *   1 2 3 4 5 6 7 8 9 10
     *
     * N = 10
     *
     * p01:
     *   ceil(10 * 0.01) = 1
     *
     * p50:
     *   ceil(10 * 0.50) = 5
     *
     * p90:
     *   ceil(10 * 0.90) = 9
     *
     * p99:
     *   ceil(10 * 0.99) = 10
     */
    for (uint64_t i = 1; i <= 10; ++i)
    {
        stats.record(i);
    }

    EXPECT_EQ(stats.percentile(0.01), 1);
    EXPECT_EQ(stats.percentile(0.10), 1);
    EXPECT_EQ(stats.percentile(0.20), 2);

    EXPECT_EQ(stats.percentile(0.50), 5);

    EXPECT_EQ(stats.percentile(0.90), 9);
    EXPECT_EQ(stats.percentile(0.99), 10);
    EXPECT_EQ(stats.percentile(1.00), 10);
}


TEST(LatencyStatsTest, P90AndP99WithFourSamples)
{
    LatencyStats stats;

    stats.record(1);
    stats.record(2);
    stats.record(3);
    stats.record(4);

    /*
     * N = 4
     *
     * p90 rank = ceil(3.6)  = 4
     * p99 rank = ceil(3.96) = 4
     *
     * Therefore p90 == p99.
     *
     * This explicitly tests the behaviour seen in the dashboard when
     * cnt = 4.
     */
    EXPECT_EQ(stats.percentile(0.50), 2);
    EXPECT_EQ(stats.percentile(0.90), 4);
    EXPECT_EQ(stats.percentile(0.99), 4);
}


TEST(LatencyStatsTest, PercentileIndependentOfInsertionOrder)
{
    LatencyStats a;
    LatencyStats b;

    const uint64_t values[] =
    {
        1, 10, 50, 100, 500, 1000, 5000
    };

    for (uint64_t value : values)
    {
        a.record(value);
    }

    for (auto it = std::rbegin(values);
         it != std::rend(values);
         ++it)
    {
        b.record(*it);
    }

    EXPECT_EQ(a.percentile(0.50), b.percentile(0.50));
    EXPECT_EQ(a.percentile(0.90), b.percentile(0.90));
    EXPECT_EQ(a.percentile(0.99), b.percentile(0.99));
    EXPECT_EQ(a.percentile(1.00), b.percentile(1.00));
}


/*
 * This specifically verifies the fix for the old snapshot bug.
 *
 * percentile() must calculate total from its histogram snapshot.
 * It must NOT depend on `count`.
 */
TEST(LatencyStatsTest, PercentileDoesNotDependOnCount)
{
    LatencyStats stats;

    stats.record(10);
    stats.record(11);
    stats.record(12);

    // Intentionally make count inconsistent with the histogram.
    stats.count.store(
        1'000'000,
        std::memory_order_relaxed
    );

    EXPECT_EQ(stats.percentile(0.50), 11);
    EXPECT_EQ(stats.percentile(0.99), 12);
}


/*
 * Same idea for max_delay_ns.
 *
 * The old implementation could fall back to max_delay_ns when count
 * and bucket snapshots did not match.
 *
 * New percentile() must be based exclusively on the captured histogram.
 */
TEST(LatencyStatsTest, PercentileDoesNotFallbackToMaxDelay)
{
    LatencyStats stats;

    stats.record(10);
    stats.record(11);
    stats.record(12);

    stats.max_delay_ns.store(
        1'000'000'000ULL,
        std::memory_order_relaxed
    );

    EXPECT_EQ(stats.percentile(0.50), 11);
    EXPECT_EQ(stats.percentile(0.99), 12);
}


TEST(LatencyStatsTest, PercentileMaximumUint64Value)
{
    LatencyStats stats;

    constexpr uint64_t MAX =
        std::numeric_limits<uint64_t>::max();

    stats.record(MAX);

    EXPECT_EQ(stats.percentile(0.50), MAX);
    EXPECT_EQ(stats.percentile(0.90), MAX);
    EXPECT_EQ(stats.percentile(0.99), MAX);
    EXPECT_EQ(stats.percentile(1.00), MAX);
}


// ============================================================================
// Invalid percentile input
// ============================================================================

TEST(LatencyStatsTest, InvalidPercentileZero)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(stats.percentile(0.0), 0);
}


TEST(LatencyStatsTest, InvalidPercentileNegative)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(stats.percentile(-0.1), 0);
}


TEST(LatencyStatsTest, InvalidPercentileGreaterThanOne)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(stats.percentile(1.01), 0);
}


TEST(LatencyStatsTest, InvalidPercentileNaN)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(
        stats.percentile(
            std::numeric_limits<double>::quiet_NaN()),
        0
    );
}


TEST(LatencyStatsTest, InvalidPercentilePositiveInfinity)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(
        stats.percentile(
            std::numeric_limits<double>::infinity()),
        0
    );
}


TEST(LatencyStatsTest, InvalidPercentileNegativeInfinity)
{
    LatencyStats stats;

    stats.record(10);

    EXPECT_EQ(
        stats.percentile(
            -std::numeric_limits<double>::infinity()),
        0
    );
}


// ============================================================================
// Concurrency
// ============================================================================

TEST(LatencyStatsTest, ConcurrentRecord)
{
    LatencyStats stats;

    constexpr int THREAD_COUNT = 8;
    constexpr int RECORDS_PER_THREAD = 10'000;

    std::vector<std::thread> threads;

    for (int t = 0; t < THREAD_COUNT; ++t)
    {
        threads.emplace_back(
            [&stats, t]()
            {
                const uint64_t value =
                    static_cast<uint64_t>(t + 1);

                for (int i = 0;
                     i < RECORDS_PER_THREAD;
                     ++i)
                {
                    stats.record(value);
                }
            }
        );
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    constexpr uint64_t EXPECTED_COUNT =
        THREAD_COUNT * RECORDS_PER_THREAD;

    // Each thread records:
    //
    // thread 0 -> 1
    // thread 1 -> 2
    // ...
    // thread 7 -> 8
    //
    // sum(1..8) = 36
    constexpr uint64_t EXPECTED_TOTAL =
        36ULL * RECORDS_PER_THREAD;

    EXPECT_EQ(
        stats.count.load(),
        EXPECTED_COUNT
    );

    EXPECT_EQ(
        stats.total_delay_ns.load(),
        EXPECTED_TOTAL
    );

    EXPECT_EQ(
        stats.max_delay_ns.load(),
        8
    );

    uint64_t histogram_count = 0;

    for (int i = 0;
         i < LatencyStats::BUCKET_COUNT;
         ++i)
    {
        histogram_count +=
            stats.buckets[i].load(
                std::memory_order_relaxed);
    }

    EXPECT_EQ(
        histogram_count,
        EXPECTED_COUNT
    );

    // Equal number of samples 1..8.
    //
    // p50 rank lands at value 4.
    // p90 lands at value 8.
    EXPECT_EQ(stats.percentile(0.50), 4);
    EXPECT_EQ(stats.percentile(0.90), 8);
    EXPECT_EQ(stats.percentile(0.99), 8);
}