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
    T wait_result(std::future<T>& f, std::chrono::milliseconds timeout = 1000ms)
    {
        if (f.wait_for(timeout) != std::future_status::ready)
        {
            ADD_FAILURE() << "future timeout";
            throw std::runtime_error("future timeout");
        }

        return f.get();
    }

    inline void wait_done(std::future<void>& f, std::chrono::milliseconds timeout = 1000ms)
    {
        if (f.wait_for(timeout) != std::future_status::ready)
        {
            ADD_FAILURE() << "future<void> timeout";
            throw std::runtime_error("future<void> timeout");
        }

        f.get();
    }

    inline EventBase* test_event_base()
    {
        return EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY);
    }
}

TEST(CoroutineUsageEventBaseTest, ManyTasksOnSameEventBaseSequentialStart)
{
    constexpr int N = 1000;
    auto eb = test_event_base();

    auto fn = [](int i) -> Task<int>
    {
        co_return i;
    };

    long long sum = 0;

    for (int i = 0; i < N; ++i)
    {
        auto task = fn(i);
        auto result = task.start_running_on(eb);
        sum += wait_result(result);
    }

    ASSERT_EQ(sum, (N - 1LL) * N / 2);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEventBaseTest, ManyTasksOnSameEventBaseBurst)
{
    constexpr int N = 1000;
    auto eb = test_event_base();

    auto fn = [](int i) -> Task<int>
    {
        co_return i;
    };

    std::vector<Task<int>> tasks;
    std::vector<std::future<int>> results;
    tasks.reserve(N);
    results.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        tasks.emplace_back(fn(i));
        results.emplace_back(tasks.back().start_running_on(eb));
    }

    long long sum = 0;
    for (auto& f : results)
    {
        sum += wait_result(f, 3000ms);
    }

    ASSERT_EQ(sum, (N - 1LL) * N / 2);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEventBaseTest, ParentAwaitsChildInsteadOfBlockingFutureGet)
{
    auto child = []() -> Task<int>
    {
        co_return 42;
    };

    auto parent = [&]() -> Task<int>
    {
        int v = co_await child();
        co_return v;
    };

    auto task = parent();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEventBaseTest, TaskLoopAwaitManyTimes)
{
    auto step = [](int x) -> Task<int>
    {
        co_return x + 1;
    };

    auto fn = [&]() -> Task<int>
    {
        int v = 0;
        for (int i = 0; i < 42; ++i)
        {
            v = co_await step(v);
        }
        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEventBaseTest, DifferentEventBaseIdsCanRunTasks)
{
    auto eb1 = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER);
    auto eb2 = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER_BOOK);

    auto fn = []() -> Task<int>
    {
        co_return 21;
    };

    auto t1 = fn();
    auto t2 = fn();

    auto f1 = t1.start_running_on(eb1);
    auto f2 = t2.start_running_on(eb2);

    ASSERT_EQ(wait_result(f1) + wait_result(f2), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
