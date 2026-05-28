#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>
#include <memory>
#include <utility>

#include <coroutine/task.h>
#include <coroutine/future.h>
#include <coroutine/event_base_manager.h>

using namespace std::chrono_literals;

namespace
{
    template <class T>
    T wait_result(std::future<T>& f, std::chrono::milliseconds timeout = 1000ms)
    {
        EXPECT_EQ(f.wait_for(timeout), std::future_status::ready);
        return f.get();
    }

    inline void wait_done(std::future<void>& f, std::chrono::milliseconds timeout = 1000ms)
    {
        EXPECT_EQ(f.wait_for(timeout), std::future_status::ready);
        f.get();
    }

    inline EventBase* test_event_base()
    {
        // Use a non-IO event base for black-box coroutine tests.
        return EventBaseManager::get_event_base_by_id(EventBaseID::NO_STRATEGY);
    }
}

TEST(CoroutineUsageBasicTaskTest, TaskIntReturnsValue)
{
    auto fn = []() -> Task<int>
    {
        co_return 42;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}

TEST(CoroutineUsageBasicTaskTest, TaskVoidCompletes)
{
    std::atomic<int> counter{0};

    auto fn = [&]() -> Task<void>
    {
        counter.fetch_add(1, std::memory_order_relaxed);
        co_return;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    wait_done(result);
    ASSERT_EQ(counter.load(std::memory_order_relaxed), 1);
}

TEST(CoroutineUsageBasicTaskTest, TaskBodyDoesNotRunBeforeScheduled)
{
    std::atomic<int> counter{0};

    auto fn = [&]() -> Task<int>
    {
        counter.fetch_add(1, std::memory_order_relaxed);
        co_return 7;
    };

    auto task = fn();

    ASSERT_EQ(counter.load(std::memory_order_relaxed), 0);

    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 7);
    ASSERT_EQ(counter.load(std::memory_order_relaxed), 1);
}

TEST(CoroutineUsageBasicTaskTest, TaskExecutesExactlyOnce)
{
    std::atomic<int> counter{0};

    auto fn = [&]() -> Task<int>
    {
        counter.fetch_add(1, std::memory_order_relaxed);
        co_return 100;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 100);
    ASSERT_EQ(counter.load(std::memory_order_relaxed), 1);
}
