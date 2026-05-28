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


TEST(CoroutineUsageCompositionTest, TaskAwaitsFutureThenReturns)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(40);
        });

        co_return v + 2;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}

TEST(CoroutineUsageCompositionTest, ChildTaskAwaitsFuture)
{
    auto child = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(21);
        });

        co_return v;
    };

    auto parent = [&]() -> Task<int>
    {
        int v = co_await child();
        co_return v * 2;
    };

    auto task = parent();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}

TEST(CoroutineUsageCompositionTest, ChainTaskFutureTaskFuture)
{
    auto leaf = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(2);
        });

        co_return v;
    };

    auto root = [&]() -> Task<int>
    {
        int a = co_await Future<int>([](auto* out)
        {
            out->set_value(40);
        });

        int b = co_await leaf();

        co_return a + b;
    };

    auto task = root();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}

TEST(CoroutineUsageCompositionTest, SeveralAsyncSteps)
{
    auto fn = []() -> Task<int>
    {
        int total = 0;

        for (int i = 0; i < 6; ++i)
        {
            total += co_await Future<int>([i](auto* out)
            {
                out->set_value(i + 2);
            });
        }

        co_return total;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 27);
}
