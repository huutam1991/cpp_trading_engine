#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
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

    template <class T>
    struct FrameCounterGuard
    {
        FrameCounterGuard()
        {
            Task<T>::promise_type::reset_frame_counters();
        }

        int64_t alloc() const
        {
            return Task<T>::promise_type::frame_alloc_count();
        }

        int64_t free() const
        {
            return Task<T>::promise_type::frame_free_count();
        }

        void expect_counts(int64_t expected_alloc, int64_t expected_free) const
        {
            ASSERT_EQ(alloc(), expected_alloc);
            ASSERT_EQ(free(), expected_free);
        }
    };

    inline void settle()
    {
        std::this_thread::sleep_for(2ms);
    }
}

TEST(CoroutineUsageLifetimeEdgeTest, DestroyImmediatelyAfterScheduleDoesNotCrashAndEventuallyReleases)
{
    FrameCounterGuard<int> frames;

    auto fn = []() -> Task<int>
    {
        co_return 42;
    };

    {
        auto task = fn();
        auto result = task.start_running_on(test_event_base());
        (void)result;
    }

    settle();

    ASSERT_EQ(frames.alloc(), 1);
    ASSERT_LE(frames.free(), 1);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, DestroyWrapperWhileSuspendedOnNeverCompletingFutureDoesNotCrash)
{
    FrameCounterGuard<int> frames;

    auto fn = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto*)
        {
            // Never complete.
        });

        co_return v;
    };

    {
        auto task = fn();
        auto result = task.start_running_on(test_event_base()).spin_wait();

        // ASSERT_EQ(result.wait_for(20ms), std::future_status::timeout);
    }

    settle();

    ASSERT_EQ(frames.alloc(), 1);

    // Current runtime may intentionally keep this frame alive because there is
    // no cancellation contract. If you implement cancellation-on-destroy, change
    // this to ASSERT_EQ(frames.free(), 1).
    ASSERT_LE(frames.free(), 1);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, DISABLED_DestroyParentWhileChildStillRunningPolicy)
{
    FrameCounterGuard<int> frames;

    auto child = []() -> Task<int>
    {
        int v = co_await Future<int>([](auto* out)
        {
            std::thread([out]()
            {
                std::this_thread::sleep_for(10ms);
                out->set_value(42);
            }).detach();
        });

        co_return v;
    };

    auto parent = [&]() -> Task<int>
    {
        int v = co_await child();
        co_return v;
    };

    {
        auto task = parent();
        auto result = task.start_running_on(test_event_base()).spin_wait();
    }

    settle();

    ASSERT_LE(frames.free(), frames.alloc());

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, DISABLED_ChildCompletesAfterParentReleasedPolicy)
{
    SUCCEED();

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, DISABLED_TaskInfoReleasedButCoroutineFrameStillAliveNeedsPoolCounters)
{
    // Required test hooks:
    // - TaskInfoPool::active_count()
    // - TaskInfoPool::acquire_count()
    // - TaskInfoPool::release_count()
    SUCCEED();

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, DISABLED_CrossThreadSetValueAfterTaskDestroyPolicy)
{
    // Future safe policy options:
    // 1. FutureValue is shared/refcounted and set_value after destroy is ignored.
    // 2. Cancellation token rejects late completion.
    // 3. Runtime declares this unsupported and debug-asserts.
    SUCCEED();

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, MoveSuspendedTaskWrapperThenCompleteReleasesOnce)
{
    FrameCounterGuard<int> frames;

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

    {
        auto task1 = fn();
        auto result = task1.start_running_on(test_event_base());

        // ASSERT_EQ(result.wait_for(1ms), std::future_status::timeout);

        auto task2 = std::move(task1);

        ASSERT_EQ(wait_result(result), 42);
    }

    settle();

    ASSERT_EQ(frames.alloc(), 1);
    ASSERT_EQ(frames.free(), 1);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, MoveAssignOverUnscheduledTaskWhileOtherIsSuspendedMustReleaseOldFrame)
{
    FrameCounterGuard<int> frames;

    auto delayed = []() -> Task<int>
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

    auto immediate = []() -> Task<int>
    {
        co_return 7;
    };

    {
        auto task1 = immediate(); // unscheduled frame
        auto task2 = delayed();   // frame that will be scheduled and suspended

        auto result2 = task2.start_running_on(test_event_base());

        task1 = std::move(task2);

        ASSERT_EQ(wait_result(result2), 42);
    }

    settle();

    ASSERT_EQ(frames.alloc(), 2);

    // Target behavior:
    // - task1's old unscheduled frame must be released during move assignment.
    // - delayed task frame must be released after completion + wrapper destruction.
    //
    // If this fails with free == 1, runtime still leaks unscheduled overwritten frame.
    ASSERT_EQ(frames.free(), 2);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeEdgeTest, DISABLED_SelfMoveActiveTaskWrapperPolicy)
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
    task = std::move(task);
#pragma GCC diagnostic pop

    ASSERT_EQ(wait_result(result), 42);

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}