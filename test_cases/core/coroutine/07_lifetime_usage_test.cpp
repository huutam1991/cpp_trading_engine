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

TEST(CoroutineUsageLifetimeTest, DestroyTaskBeforeSchedulingIsSafe)
{
    auto fn = []() -> Task<int>
    {
        co_return 42;
    };

    {
        auto task = fn();
    }

    SUCCEED();
}

TEST(CoroutineUsageLifetimeTest, DestroyTaskAfterCompletionIsSafe)
{
    auto fn = []() -> Task<int>
    {
        co_return 42;
    };

    {
        auto task = fn();
        auto result = task.start_running_on(test_event_base());
        ASSERT_EQ(wait_result(result), 42);
    }

    SUCCEED();
}

TEST(CoroutineUsageLifetimeTest, MoveTaskThenRun)
{
    auto fn = []() -> Task<int>
    {
        co_return 42;
    };

    auto task1 = fn();
    auto task2 = std::move(task1);

    auto result = task2.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}

TEST(CoroutineUsageLifetimeTest, MoveAssignOverInactiveTask)
{
    auto fn = [](int v) -> Task<int>
    {
        co_return v;
    };

    auto task1 = fn(1);
    auto task2 = fn(42);

    task1 = std::move(task2);

    auto result = task1.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}

TEST(CoroutineUsageLifetimeTest, TemporaryTaskReturnedFromFunctionWorks)
{
    auto make_task = []() -> Task<int>
    {
        co_return 42;
    };

    auto task = make_task();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);
}
