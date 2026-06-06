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
        return result.get();
    }

    inline void wait_done(TaskResult<void>& result)
    {
        result.get();
    }

    inline EventBase* test_event_base()
    {
        return EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY);
    }
}

TEST(CoroutineUsageAwaitTaskTest, ParentAwaitsChildValue)
{
    auto child = []() -> Task<int>
    {
        co_return 21;
    };

    auto parent = [&]() -> Task<int>
    {
        int v = co_await child();
        co_return v * 2;
    };

    auto task = parent();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageAwaitTaskTest, ParentAwaitsVoidChild)
{
    std::atomic<bool> child_ran{false};

    auto child = [&]() -> Task<void>
    {
        child_ran.store(true, std::memory_order_relaxed);
        co_return;
    };

    auto parent = [&]() -> Task<int>
    {
        co_await child();
        co_return child_ran.load(std::memory_order_relaxed) ? 1 : 0;
    };

    auto task = parent();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 1);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageAwaitTaskTest, NestedTaskChain)
{
    auto c = []() -> Task<int> { co_return 10; };

    auto b = [&]() -> Task<int>
    {
        int v = co_await c();
        co_return v + 20;
    };

    auto a = [&]() -> Task<int>
    {
        int v = co_await b();
        co_return v + 12;
    };

    auto task = a();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageAwaitTaskTest, MultipleSequentialAwaits)
{
    auto value = [](int x) -> Task<int>
    {
        co_return x;
    };

    auto parent = [&]() -> Task<int>
    {
        int a = co_await value(10);
        int b = co_await value(20);
        int c = co_await value(12);
        co_return a + b + c;
    };

    auto task = parent();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
