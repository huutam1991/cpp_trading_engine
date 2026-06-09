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

    inline void small_settle_delay()
    {
        std::this_thread::sleep_for(1ms);
    }

    inline void large_settle_delay()
    {
        std::this_thread::sleep_for(10ms);
    }
}

TEST(CoroutineUsageLifetimeTest, DestroyTaskBeforeSchedulingReleasesCoroutineFrame)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        {
            auto task = fn();
            frames.expect_counts(1, 0);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, DestroyVoidTaskBeforeSchedulingReleasesCoroutineFrame)
{
    {
        FrameCounterGuard<void> frames;

        auto fn = []() -> Task<void>
        {
            co_return;
        };

        {
            auto task = fn();
            frames.expect_counts(1, 0);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, DestroyTaskAfterCompletionReleasesCoroutineFrame)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        {
            auto task = fn();
            frames.expect_counts(1, 0);

            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);

            ASSERT_EQ(frames.alloc(), 1);
            ASSERT_LE(frames.free(), 1);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, DestroyVoidTaskAfterCompletionReleasesCoroutineFrame)
{
    {
        FrameCounterGuard<void> frames;
        std::atomic<int> ran{0};

        auto fn = [&]() -> Task<void>
        {
            ran.fetch_add(1, std::memory_order_relaxed);
            co_return;
        };

        {
            auto task = fn();
            frames.expect_counts(1, 0);

            auto result = task.start_running_on(test_event_base());
            wait_done(result);

            ASSERT_EQ(ran.load(std::memory_order_relaxed), 1);
            ASSERT_EQ(frames.alloc(), 1);
            ASSERT_LE(frames.free(), 1);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, RepeatedCreateDestroyBeforeSchedulingReleasesEveryFrame)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = [](int v) -> Task<int>
        {
            co_return v;
        };

        constexpr int N = 1000;

        for (int i = 0; i < N; ++i)
        {
            auto task = fn(i);
        }

        small_settle_delay();
        frames.expect_counts(N, N);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, RepeatedCreateRunDestroyReleasesEveryFrame)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = [](int v) -> Task<int>
        {
            co_return v;
        };

        constexpr int N = 1000;

        for (int i = 0; i < N; ++i)
        {
            auto task = fn(i);
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), i);
        }

        small_settle_delay();
        frames.expect_counts(N, N);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, MoveConstructedTaskReleasesOnlyOnceAfterRun)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        {
            auto task1 = fn();
            frames.expect_counts(1, 0);

            auto task2 = std::move(task1);
            frames.expect_counts(1, 0);

            auto result = task2.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, MovedFromTaskDestructorDoesNotReleaseFrameOwnedByMovedToTask)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        Task<int> moved_to;

        {
            auto moved_from = fn();
            frames.expect_counts(1, 0);
            moved_to = std::move(moved_from);
        }

        frames.expect_counts(1, 0);

        auto result = moved_to.start_running_on(test_event_base());
        ASSERT_EQ(wait_result(result), 42);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, MovedToTaskReleasesFrameWhenDestroyed)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        {
            Task<int> moved_to;

            {
                auto moved_from = fn();
                moved_to = std::move(moved_from);
            }

            auto result = moved_to.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, MoveAssignOverCompletedTaskReleasesOldAndNewFrames)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = [](int v) -> Task<int>
        {
            co_return v;
        };

        {
            auto task1 = fn(1);
            auto result1 = task1.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result1), 1);

            auto task2 = fn(42);
            frames.expect_counts(2, 0);

            task1 = std::move(task2);
            small_settle_delay();

            ASSERT_EQ(frames.alloc(), 2);
            ASSERT_GE(frames.free(), 1);

            auto result2 = task1.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result2), 42);
        }

        small_settle_delay();
        frames.expect_counts(2, 2);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, MoveAssignOverUnscheduledTaskReleasesOldFrame)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = [](int v) -> Task<int>
        {
            co_return v;
        };

        {
            auto task1 = fn(1);
            auto task2 = fn(42);

            frames.expect_counts(2, 0);

            task1 = std::move(task2);

            ASSERT_EQ(frames.alloc(), 2);
            ASSERT_GE(frames.free(), 1);

            auto result = task1.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(2, 2);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, TemporaryTaskReturnedFromFunctionReleasesFrame)
{
    {
        FrameCounterGuard<int> frames;

        auto make_task = []() -> Task<int>
        {
            co_return 42;
        };

        {
            auto task = make_task();
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, TaskStoredInVectorAndMovedByReallocationReleasesFrames)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = [](int v) -> Task<int>
        {
            co_return v;
        };

        constexpr int N = 128;

        {
            std::vector<Task<int>> tasks;
            std::vector<std::future<int>> results;
            results.reserve(N);

            auto eb = test_event_base();

            for (int i = 0; i < N; ++i)
            {
                tasks.emplace_back(fn(i));
                results.emplace_back(tasks.back().start_running_on(eb));
            }

            long long sum = 0;
            for (auto& result : results)
            {
                sum += wait_result(result);
            }

            ASSERT_EQ(sum, (N - 1LL) * N / 2);
            ASSERT_EQ(frames.alloc(), N);
            ASSERT_LE(frames.free(), N);
        }

        small_settle_delay();
        frames.expect_counts(N, N);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, ParentAwaitChildReleasesBothFrames)
{
    {
        FrameCounterGuard<int> frames;

        auto child = []() -> Task<int>
        {
            co_return 21;
        };

        auto parent = [&]() -> Task<int>
        {
            int v = co_await child();
            co_return v * 2;
        };

        {
            auto task = parent();
            ASSERT_EQ(frames.alloc(), 1);
            ASSERT_EQ(frames.free(), 0);

            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(2, 2);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, DeepAwaitChainReleasesAllFrames)
{
    {
        FrameCounterGuard<int> frames;

        auto leaf = [](int v) -> Task<int>
        {
            co_return v + 1;
        };

        auto root = [&]() -> Task<int>
        {
            int v = 0;
            for (int i = 0; i < 10; ++i)
            {
                v = co_await leaf(v);
            }

            co_return v;
        };

        {
            auto task = root();
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 10);
        }

        small_settle_delay();
        frames.expect_counts(11, 11);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, TaskAwaitingFutureReleasesFrameAfterCompletion)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            int v = co_await Future<int>([](auto out)
            {
                out.set_value(42);
            });

            co_return v;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, TaskAwaitingFutureFromThreadReleasesFrameAfterCompletion)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            int v = co_await Future<int>([](auto out)
            {
                std::thread([out = std::move(out)]() mutable
                {
                    std::this_thread::sleep_for(1ms);
                    out.set_value(42);
                }).detach();
            });

            co_return v;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeTest, PendingFutureTaskFrameNotFreedWhileTaskWrapperAlive)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            int v = co_await Future<int>([](auto)
            {
                // Intentionally never complete.
            });

            co_return v;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_event_base());

            frames.expect_counts(1, 0);
        }

        small_settle_delay();

        // Current runtime may intentionally keep a released-but-suspended frame until
        // completion/cancellation is defined. If cancellation-on-destroy is added,
        // tighten this to expect_counts(1, 1).
        ASSERT_EQ(frames.alloc(), 1);
        ASSERT_LE(frames.free(), 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

// Enable after cancellation semantics are explicitly defined.
TEST(CoroutineUsageLifetimeTest, DestroyRunningTaskWaitingOnFutureThenExternalCompletionIsSafe)
{
    {
        FrameCounterGuard<int> frames;
        Future<int>::FutureValue out;

        std::promise<void> captured;
        auto captured_future = captured.get_future();

        auto fn = [&]() -> Task<int>
        {
            int v = co_await Future<int>([&](auto value)
            {
                out = std::move(value);
                captured.set_value();
            });

            co_return v;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_event_base());

            captured_future.get();
        }

        out.set_value(42);

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

// Enable if self-move is officially supported by Task.
TEST(CoroutineUsageLifetimeTest, SelfMoveAssignmentPolicy)
{
    {
        FrameCounterGuard<int> frames;

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        {
            auto task = fn();
            frames.expect_counts(1, 0);

            task = std::move(task);

            auto result = task.start_running_on(test_event_base());
            ASSERT_EQ(wait_result(result), 42);
        }

        small_settle_delay();
        frames.expect_counts(1, 1);
    }

    // Cleanup event base threads after test
    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageLifetimeStressTest, ManyTasksHundredThreadsComplexSuspendLifetime)
{
    {
        FrameCounterGuard<int> frames;

        constexpr int THREADS = 100;
        constexpr int TASKS_PER_THREAD = 10000;
        constexpr int TOTAL_TASKS = THREADS * TASKS_PER_THREAD;

        std::atomic<int64_t> completed{0};

        auto leaf = [](int v) -> Task<int>
        {
            int r = co_await Future<int>([v](auto out) mutable
            {
                // No thread-per-future here. Complete directly.
                out.set_value(v + 1);
            });

            co_return r;
        };

        auto mid = [&]() -> Task<int>
        {
            int a = co_await leaf(1);
            int b = co_await leaf(a);
            int c = co_await leaf(b);

            co_return c;
        };

        auto root = [&]() -> Task<int>
        {
            int v1 = co_await mid();
            int v2 = co_await leaf(v1);
            int v3 = co_await leaf(v2);

            co_return v3;
        };

        auto worker = [&]()
        {
            auto eb = test_event_base();

            std::vector<Task<int>> tasks;
            std::vector<std::future<int>> results;

            tasks.reserve(128);
            results.reserve(128);

            for (int i = 0; i < TASKS_PER_THREAD; ++i)
            {
                tasks.emplace_back(root());
                results.emplace_back(tasks.back().start_running_on(eb));

                if (tasks.size() >= 128)
                {
                    for (auto& result : results)
                    {
                        ASSERT_EQ(wait_result(result), 6);
                        completed.fetch_add(1, std::memory_order_relaxed);
                    }

                    results.clear();
                    tasks.clear();
                }
            }

            for (auto& result : results)
            {
                ASSERT_EQ(wait_result(result), 6);
                completed.fetch_add(1, std::memory_order_relaxed);
            }

            results.clear();
            tasks.clear();
        };

        std::vector<std::thread> threads;
        threads.reserve(THREADS);

        for (int i = 0; i < THREADS; ++i)
        {
            threads.emplace_back(worker);
        }

        for (auto& t : threads)
        {
            t.join();
        }

        large_settle_delay();

        ASSERT_EQ(completed.load(std::memory_order_relaxed), TOTAL_TASKS);

        // Per root task:
        // root = 1 frame
        // mid  = 1 frame
        // leaf inside mid = 3 frames
        // leaf inside root = 2 frames
        //
        // total = 7 Task<int> frames per root task
        frames.expect_counts(TOTAL_TASKS * 7LL, TOTAL_TASKS * 7LL);
    }

    EventBaseManager::shutdown_all();
}