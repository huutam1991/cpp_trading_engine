#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>
#include <memory>
#include <utility>
#include <iostream>

#include <coroutine/task.h>
#include <coroutine/future.h>
#include <coroutine/event_base_manager.h>

using namespace std::chrono_literals;

namespace
{
    template <class T>
    T wait_result(std::future<T>& result)
    {
        return result.get();
    }

    inline void wait_done(std::future<void>& result)
    {
        result.get();
    }

    inline EventBase* test_event_base()
    {
        return EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY);
    }
}

TEST(CoroutineUsageBasicTaskTest, TaskIntReturnsValue)
{
    {
        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        auto task = fn();
        auto result = task.start_running_on(test_event_base());

        ASSERT_EQ(wait_result(result), 42);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageBasicTaskTest, TaskVoidCompletes)
{
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

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageBasicTaskTest, TaskBodyDoesNotRunBeforeScheduled)
{
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

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageBasicTaskTest, TaskExecutesExactlyOnce)
{
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

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
