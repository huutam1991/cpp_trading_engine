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
        return EventBaseManager::get_event_base_by_id(EventBaseID::NO_STRATEGY);
    }
}


TEST(CoroutineUsagePerformanceTest, SimpleTaskDispatchLatencyBudget)
{
    constexpr int N = 100000;

    auto fn = []() -> Task<int>
    {
        co_return 1;
    };

    auto eb = test_event_base();

    auto start = std::chrono::high_resolution_clock::now();

    int total = 0;
    for (int i = 0; i < N; ++i)
    {
        auto task = fn();
        auto result = task.start_running_on(eb);
        total += wait_result(result, 5000ms);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(ns) / N;

    std::cout << "[perf] simple task avg ns: " << avg_ns << std::endl;

    ASSERT_EQ(total, N);
    ASSERT_LT(avg_ns, 100000.0);
}

TEST(CoroutineUsagePerformanceTest, TaskAwaitTaskLatencyBudget)
{
    constexpr int N = 100000;

    auto child = []() -> Task<int>
    {
        co_return 1;
    };

    auto parent = [&]() -> Task<int>
    {
        int v = co_await child();
        co_return v;
    };

    auto eb = test_event_base();

    auto start = std::chrono::high_resolution_clock::now();

    int total = 0;
    for (int i = 0; i < N; ++i)
    {
        auto task = parent();
        auto result = task.start_running_on(eb);
        total += wait_result(result, 5000ms);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(ns) / N;

    std::cout << "[perf] task-await-task avg ns: " << avg_ns << std::endl;

    ASSERT_EQ(total, N);
    ASSERT_LT(avg_ns, 100000.0);
}

TEST(CoroutineUsagePerformanceTest, FutureImmediateWakeupLatencyBudget)
{
    constexpr int N = 100000;

    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(1);
        });

        co_return v;
    };

    auto eb = test_event_base();

    auto start = std::chrono::high_resolution_clock::now();

    int total = 0;
    for (int i = 0; i < N; ++i)
    {
        auto task = fn();
        auto result = task.start_running_on(eb);
        total += wait_result(result, 5000ms);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(ns) / N;

    std::cout << "[perf] future immediate wakeup avg ns: " << avg_ns << std::endl;

    ASSERT_EQ(total, N);
    ASSERT_LT(avg_ns, 100000.0);
}
