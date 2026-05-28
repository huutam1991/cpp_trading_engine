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

#include <stdexcept>

// Current implementation may terminate on unhandled_exception.
// Keep these as disabled death tests until desired failure policy is finalized.

TEST(CoroutineUsageFailureTest, FutureNeverCompletesLeavesTaskPendingSafely)
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

    ASSERT_EQ(result.wait_for(20ms), std::future_status::timeout);
}

TEST(CoroutineUsageFailureTest, DISABLED_ExceptionBeforeAwaitPolicy)
{
    auto fn = []() -> Task<int>
    {
        throw std::runtime_error("boom");
        co_return 42;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    // Choose one policy:
    // 1. EXPECT_DEATH(...), if unhandled exception terminates.
    // 2. EXPECT_THROW(...), if refactor captures exception into result future.
    (void)result;
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
}
