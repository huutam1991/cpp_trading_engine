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

TEST(CoroutineUsageRaceTest, FutureCompletesImmediatelyNoLostWakeup)
{
    {
        constexpr int N = 10000;

        auto fn = []() -> Task<int>
        {
            int v = co_await Future<int>([](auto out)
            {
                out.set_value(1);
            });

            co_return v;
        };

        for (int i = 0; i < N; ++i)
        {
            auto task = fn();
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 1);
        }
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageRaceTest, FutureCompletesFromThreadNoLostWakeup)
{
    {
        constexpr int N = 20;

        auto fn = []() -> Task<int>
        {
            int v = co_await Future<int>([](auto out)
            {
                std::thread([out = std::move(out)]() mutable
                {
                    out.set_value(1);
                }).detach();
            });

            co_return v;
        };

        for (int i = 0; i < N; ++i)
        {
            auto task = fn();
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 1);
        }
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageRaceTest, ManyFuturesCompleteFromThreadsSequential)
{
    {
        constexpr int N = 20;

        auto fn = [](int i) -> Task<int>
        {
            int v = co_await Future<int>([i](auto out)
            {
                std::thread([out = std::move(out), i]() mutable
                {
                    out.set_value(i);
                }).detach();
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
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageRaceTest, ManyFuturesCompleteFromThreadsBurst)
{
    {
        constexpr int N = 512;

        std::vector<Task<int>> tasks;
        std::vector<std::future<int>> results;

        tasks.reserve(N);
        results.reserve(N);

        auto fn = [](int i) -> Task<int>
        {
            int v = co_await Future<int>([i](auto out)
            {
                std::thread([out = std::move(out), i]() mutable
                {
                    out.set_value(i);
                }).detach();
            });

            co_return v;
        };

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

    std::this_thread::sleep_for(10ms);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
