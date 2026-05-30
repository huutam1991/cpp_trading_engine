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
        return result.spin_wait();
    }

    inline void wait_done(TaskResult<void>& result)
    {
        result.spin_wait();
    }

    inline EventBase* test_event_base()
    {
        return EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY);
    }
}


TEST(CoroutineUsageCancellationLikeTest, DestroyUnscheduledSuspendedStyleTaskIsSafe)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto*)
        {
            // Not completed.
        });

        co_return v;
    };

    {
        auto task = fn();
    }

    SUCCEED();

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageCancellationLikeTest, RunningTaskCanCompleteAfterDelayedFuture)
{
    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            std::thread([out]()
            {
                std::this_thread::sleep_for(5ms);
                out->set_value(42);
            }).detach();
        });

        co_return v;
    };

    auto task = fn();
    auto result = task.start_running_on(test_event_base());

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageCancellationLikeTest, RepeatedDestroyBeforeSchedulingIsSafe)
{
    auto fn = []() -> Task<int>
    {
        co_return 42;
    };

    for (int i = 0; i < 1000; ++i)
    {
        auto task = fn();
    }

    SUCCEED();

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

// This is intentionally disabled because completing a Future after destroying
// the coroutine that owns the FutureValue requires an explicit cancellation contract.
// Enable this only after the runtime defines safe external completion after task destruction.
TEST(CoroutineUsageCancellationLikeTest, DISABLED_DestroyRunningTaskThenExternalCompletionPolicy)
{
    SUCCEED();

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}
