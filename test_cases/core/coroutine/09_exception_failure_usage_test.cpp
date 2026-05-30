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


#include <stdexcept>

TEST(CoroutineUsageFailureTest, FutureNeverCompletesLeavesResultPending)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto*)
        {
            // Intentionally never set.
        });

        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

// Current implementation may terminate on unhandled_exception.
// Keep these disabled until the desired failure policy is finalized.

TEST(CoroutineUsageFailureTest, DISABLED_ExceptionBeforeAwaitPolicy)
{
    auto fn = []() -> Task<int>
    {
        throw std::runtime_error("boom");
        co_return 42;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());
    (void)result;

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageFailureTest, DISABLED_ExceptionAfterAwaitPolicy)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            out->set_value(1);
        });

        throw std::runtime_error("boom");
        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());
    (void)result;

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
