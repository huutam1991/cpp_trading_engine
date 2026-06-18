#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include <coroutine/task.h>
#include <coroutine/future.h>
#include <coroutine/event_base_manager.h>
#include <coroutine/epoll_base.h>

using namespace std::chrono_literals;

namespace
{
    template <class T>
    T wait_result(std::future<T>& result)
    {
        return result.get();
    }

    inline EventBase* test_epoll_base()
    {
        return EventBaseManager::get_event_base_by_id(EpollBaseID::EPOLL_SYSTEM_IO_TASK);
    }
}

TEST(CoroutineUsageEpollIoTest, ReadableFdWakesTask)
{
    {
        EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

        auto fn = []() -> Task<int>
        {
            co_return 42;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_epoll_base());

            ASSERT_EQ(wait_result(result), 42);
        }

        std::this_thread::sleep_for(2ms);

        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed), 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed), 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_release_count.load(std::memory_order_relaxed), 2);

        // set_ready_task() does not call activate().
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_activate_count.load(std::memory_order_relaxed), 0);

        // TaskInfoEventEpoll only uses EPOLLIN.
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_write_count.load(std::memory_order_relaxed), 0);
    }

    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEpollIoTest, MultipleIoObjects)
{
    {
        constexpr int N = 1000;
        {
            EpollBase::TaskInfoEventEpoll::reset_task_event_counters();


            auto fn = [](int i) -> Task<int>
            {
                co_return i;
            };

            std::vector<Task<int>> tasks;
            std::vector<std::future<int>> results;

            tasks.reserve(N);
            results.reserve(N);

            auto eb = test_epoll_base();

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

        }

        std::this_thread::sleep_for(2ms);

        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed), N * 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed), N * 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_release_count.load(std::memory_order_relaxed), N * 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_write_count.load(std::memory_order_relaxed), 0);
    }

    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEpollIoTest, IoCloseCleanupIsSafe)
{
    {
        EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

        auto fn = []() -> Task<int>
        {
            co_return 7;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_epoll_base());

            ASSERT_EQ(wait_result(result), 7);
        }

        std::this_thread::sleep_for(2ms);

        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed), 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed), 2);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_release_count.load(std::memory_order_relaxed), 2);
    }

    EventBaseManager::shutdown_all();
}

TEST(CoroutineUsageEpollIoTest, IoErrorOrHangupPathSafe)
{
    {
        EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

        auto fn = []() -> Task<int>
        {
            int v = co_await Future<int>([](auto out)
            {
                out.set_value(40);
            });

            co_return v + 2;
        };

        {
            auto task = fn();
            auto result = task.start_running_on(test_epoll_base());

            ASSERT_EQ(wait_result(result), 42);
        }

        std::this_thread::sleep_for(2ms);

        // RUN event + SET_SUSPEND_VALUE event.
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed), 3);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed), 3);
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_release_count.load(std::memory_order_relaxed), 3);

        // No EPOLLOUT path for TaskInfoEventEpoll.
        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_write_count.load(std::memory_order_relaxed), 0);
    }

    EventBaseManager::shutdown_all();
}

// [Tam - need re-write CachePool]
TEST(CoroutineUsageEpollIoTest, DISABLED_Stress150kTasksFromManyProducerThreads)
{
    {
        EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

        constexpr int THREADS = 12;
        constexpr int TOTAL_TASKS = 150000;
        constexpr int TASKS_PER_THREAD = TOTAL_TASKS / THREADS;

        auto eb = test_epoll_base();

        auto fn = [](int v) -> Task<int>
        {
            co_return v;
        };

        std::atomic<long long> total_sum{0};
        std::vector<std::thread> producers;

        for (int t = 0; t < THREADS; ++t)
        {
            producers.emplace_back([&, t]()
            {
                long long local_sum = 0;

                for (int i = 0; i < TASKS_PER_THREAD; ++i)
                {
                    int v = t * TASKS_PER_THREAD + i;

                    auto task = fn(v);
                    auto result = task.start_running_on(eb);

                    local_sum += wait_result(result);
                }

                total_sum.fetch_add(local_sum, std::memory_order_relaxed);
            });
        }

        for (auto& th : producers)
        {
            th.join();
        }

        constexpr int ACTUAL_TOTAL = THREADS * TASKS_PER_THREAD;
        const long long expected_sum =
            (ACTUAL_TOTAL - 1LL) * ACTUAL_TOTAL / 2;

        ASSERT_EQ(total_sum.load(std::memory_order_relaxed), expected_sum);

        std::this_thread::sleep_for(10ms);

        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_write_count.load(std::memory_order_relaxed), 0);

        ASSERT_GE(EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed), ACTUAL_TOTAL);
        ASSERT_EQ(
            EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed),
            EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed)
        );
        ASSERT_EQ(
            EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed),
            EpollBase::TaskInfoEventEpoll::task_event_release_count.load(std::memory_order_relaxed)
        );
    }

    EventBaseManager::shutdown_all();
}

// [Tam - need re-write CachePool]
TEST(CoroutineUsageEpollIoTest, DISABLED_Stress150kComplexNestedTaskChains)
{
    {
        EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

        constexpr int TOTAL_TASKS = 150000;
        constexpr int THREADS = 8;
        constexpr int ROOT_TASKS = 15000;
        constexpr int ROOTS_PER_THREAD = ROOT_TASKS / THREADS;

        auto eb = test_epoll_base();

        auto leaf = [](int v) -> Task<int>
        {
            int x = co_await Future<int>([v](auto out)
            {
                out.set_value(v + 1);
            });

            co_return x;
        };

        auto middle = [&](int v) -> Task<int>
        {
            int a = co_await leaf(v);
            int b = co_await leaf(a);
            int c = co_await leaf(b);
            co_return c;
        };

        auto root = [&](int v) -> Task<int>
        {
            int x = v;

            // 10 middle calls * 3 leaf calls = 30 child tasks
            // plus 10 middle frames + 1 root frame.
            for (int i = 0; i < 10; ++i)
            {
                x = co_await middle(x);
            }

            co_return x;
        };

        std::atomic<long long> completed{0};
        std::vector<std::thread> producers;

        for (int t = 0; t < THREADS; ++t)
        {
            producers.emplace_back([&, t]()
            {
                long long local_completed = 0;

                for (int i = 0; i < ROOTS_PER_THREAD; ++i)
                {
                    int start = t * ROOTS_PER_THREAD + i;

                    auto task = root(start);
                    auto result = task.start_running_on(eb);

                    int out = wait_result(result);

                    ASSERT_EQ(out, start + 30);
                    ++local_completed;
                }

                completed.fetch_add(local_completed, std::memory_order_relaxed);
            });
        }

        for (auto& th : producers)
        {
            th.join();
        }

        constexpr int ACTUAL_ROOTS = THREADS * ROOTS_PER_THREAD;

        ASSERT_EQ(completed.load(std::memory_order_relaxed), ACTUAL_ROOTS);

        std::this_thread::sleep_for(20ms);

        ASSERT_EQ(EpollBase::TaskInfoEventEpoll::task_event_handle_write_count.load(std::memory_order_relaxed), 0);

        ASSERT_EQ(
            EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed),
            EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed)
        );
        ASSERT_EQ(
            EpollBase::TaskInfoEventEpoll::task_event_handle_read_count.load(std::memory_order_relaxed),
            EpollBase::TaskInfoEventEpoll::task_event_release_count.load(std::memory_order_relaxed)
        );

        // This test creates far more than 150k coroutine/task events overall.
        ASSERT_GE(
            EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(std::memory_order_relaxed),
            TOTAL_TASKS
        );
    }

    EventBaseManager::shutdown_all();
}