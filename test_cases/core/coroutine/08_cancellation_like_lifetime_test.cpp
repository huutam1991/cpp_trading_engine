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

    ASSERT_EQ(wait_result(result, 1000ms), 42);
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
}

// This is intentionally disabled because completing a Future after destroying
// the coroutine that owns the FutureValue requires an explicit cancellation contract.
// Enable this only after the runtime defines safe external completion after task destruction.
TEST(CoroutineUsageCancellationLikeTest, DISABLED_DestroyRunningTaskThenExternalCompletionPolicy)
{
    SUCCEED();
}
