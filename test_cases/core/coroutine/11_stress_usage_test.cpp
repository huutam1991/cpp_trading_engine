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
        return EventBaseManager::get_event_base_by_id(EventBaseID::ORDER);
    }
}

TEST(CoroutineUsageStressTest, ManySimpleTasksSequential)
{
    constexpr int N = 10000;

    auto fn = [](int i) -> Task<int>
    {
        co_return i;
    };

    long long sum = 0;

    for (int i = 0; i < N; ++i)
    {
        auto task = fn(i);
        auto result = task.start_running_on(test_event_base());
        sum += wait_result(result);
    }

    ASSERT_EQ(sum, (N - 1LL) * N / 2);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageStressTest, ManySimpleTasksBurst)
{
    {
        constexpr int N = 10000;

        auto fn = [](int i) -> Task<int>
        {
            co_return i;
        };

        std::vector<Task<int>> tasks;
        std::vector<std::future<int>> results;

        tasks.reserve(N);
        results.reserve(N);

        auto eb = test_event_base();

        for (int i = 0; i < N; ++i)
        {
            tasks.emplace_back(fn(i));
            results.emplace_back(tasks.back().start_running_on(eb));
        }

        long long sum = 0;
        for (auto& f : results)
        {
            sum += wait_result(f);
        }

        ASSERT_EQ(sum, (N - 1LL) * N / 2);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageStressTest, ManyTaskAwaitChains)
{
    constexpr int N = 1000;

    auto leaf = [](int x) -> Task<int>
    {
        co_return x + 1;
    };

    auto root = [&](int x) -> Task<int>
    {
        int v = x;
        for (int i = 0; i < 10; ++i)
        {
            v = co_await leaf(v);
        }
        co_return v;
    };

    for (int i = 0; i < N; ++i)
    {
        auto task = root(i);
        auto result = task.start_running_on(test_event_base());
        ASSERT_EQ(wait_result(result), i + 10);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageStressTest, ManyFutureWakeupsSequential)
{
    constexpr int N = 1000;

    auto fn = [](int i) -> Task<int>
    {
        int v = co_await Future<int>([i](Future<int>::FutureValue out) mutable
        {
            out.set_value(i);
        });

        co_return v;
    };

    long long sum = 0;

    for (int i = 0; i < N; ++i)
    {
        auto task = fn(i);
        auto result = task.start_running_on(test_event_base());
        sum += wait_result(result);
    }

    ASSERT_EQ(sum, (N - 1LL) * N / 2);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
