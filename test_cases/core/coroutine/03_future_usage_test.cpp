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
    T wait_result(TaskResult<T>& result)
    {
        return result.spin_wait();
    }

    inline void wait_done(TaskResult<void>& result)
    {
        result.spin_wait();
    }

    inline EventBase* test_event_base()
    {
        return EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY);
    }
}


TEST(CoroutineUsageFutureTest, AwaitFutureSetFromSameThread)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(42);
        });

        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageFutureTest, AwaitFutureSetFromAnotherThread)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            std::thread([out]()
            {
                std::this_thread::sleep_for(1ms);
                out->set_value(42);
            }).detach();
        });

        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageFutureTest, DoubleSetValueOnlyFirstWins)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(42);
            out->set_value(999);
        });

        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageFutureTest, FutureExecuteFunctionRunsOnce)
{
    std::atomic<int> execute_count{0};

    auto fn = [&]() -> Task<int>
    {
        int v = co_await Future<int>([&](auto* out)
        {
            execute_count.fetch_add(1, std::memory_order_relaxed);
            out->set_value(42);
        });

        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
    ASSERT_EQ(execute_count.load(std::memory_order_relaxed), 1);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
