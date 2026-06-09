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


#include <stdexcept>

TEST(CoroutineUsageFailureTest, FutureNeverCompletesLeavesResultPending)
{
    {
        auto fn = []() -> Task<std::string>
        {
            std::string v = co_await Future<std::string>([](auto)
            {
                // Intentionally never set.
            });

            co_return v;
        };

        auto task = fn();
        auto result = task.start_running_on(test_event_base());
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

// Current implementation may terminate on unhandled_exception.
// Keep these disabled until the desired failure policy is finalized.

TEST(CoroutineUsageFailureTest, ExceptionBeforeAwaitPolicy)
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

TEST(CoroutineUsageFailureTest, ExceptionAfterAwaitPolicy)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto out)
        {
            out.set_value(1);
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
