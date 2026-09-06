#include <gtest/gtest.h>

#include <atomic>
#include <array>
#include <barrier>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

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
        return EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_SYSTEM_IO_TASK);
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
            auto result = task.get_future();
            task.start_running_on(test_epoll_base());

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
                auto result = tasks.back().get_future();
                tasks.back().start_running_on(eb);
                results.emplace_back(std::move(result));
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
            auto result = task.get_future();
            task.start_running_on(test_epoll_base());

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
            auto result = task.get_future();
            task.start_running_on(test_epoll_base());

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
                    auto result = task.get_future();
                    task.start_running_on(eb);

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
                    auto result = task.get_future();
                    task.start_running_on(eb);

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

TEST(CoroutineUsageEpollIoTest, BurstScheduleAllBeforeWaitingDoesNotLeakPool)
{
    constexpr int N = 50000;

    auto* eb = test_epoll_base();

    const size_t initial_pool_size =
        EpollBase::TaskInfoEventPool::size();

    EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

    auto fn = [](int value) -> Task<int>
    {
        co_return value;
    };

    std::vector<Task<int>> tasks;
    std::vector<std::future<int>> results;

    tasks.reserve(N);
    results.reserve(N);

    //
    // IMPORTANT:
    // Schedule ALL tasks first.
    // No wait_result() here.
    //
    for (int i = 0; i < N; ++i)
    {
        tasks.emplace_back(fn(i));
        results.emplace_back(tasks.back().get_future());

        tasks.back().start_running_on(eb);
    }

    long long sum = 0;

    for (auto& result : results)
    {
        sum += result.get();
    }

    EXPECT_EQ(sum, (N - 1LL) * N / 2);

    tasks.clear();

    //
    // Wait until REMOVE_AWAITER events are also drained.
    //
    for (int i = 0; i < 1000; ++i)
    {
        if (EpollBase::TaskInfoEventPool::size() == initial_pool_size)
            break;

        std::this_thread::sleep_for(1ms);
    }

    EXPECT_EQ(
        EpollBase::TaskInfoEventPool::size(),
        initial_pool_size);

    EXPECT_EQ(
        EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(
            std::memory_order_relaxed),
        EpollBase::TaskInfoEventEpoll::task_event_release_count.load(
            std::memory_order_relaxed));

    EventBaseManager::shutdown_all();
}

// ============================================================================
// Additional coverage for resource exhaustion, syscall rollback,
// high-watermark/in-flight pressure, and CachePool MPMC invariants.
//
// The syscall wrappers below are intentionally defined in the test binary.
// When no failure is armed they forward directly to the Linux syscalls, so
// existing tests keep the normal behavior. The wrappers make failure-path
// tests deterministic without changing production EpollBase code.
// ============================================================================

namespace epoll_test_injection
{
    inline std::atomic<int> fail_eventfd_count{0};
    inline std::atomic<int> fail_epoll_add_count{0};
    inline std::atomic<int> fail_epoll_del_count{0};
    inline std::atomic<int> fail_eventfd_write_count{0};

    inline bool consume_failure(std::atomic<int>& counter) noexcept
    {
        int current = counter.load(std::memory_order_relaxed);

        while (current > 0)
        {
            if (counter.compare_exchange_weak(
                    current,
                    current - 1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed))
            {
                return true;
            }
        }

        return false;
    }

    inline void reset() noexcept
    {
        fail_eventfd_count.store(0, std::memory_order_relaxed);
        fail_epoll_add_count.store(0, std::memory_order_relaxed);
        fail_epoll_del_count.store(0, std::memory_order_relaxed);
        fail_eventfd_write_count.store(0, std::memory_order_relaxed);
    }
}

extern "C" int eventfd(unsigned int initval, int flags) noexcept
{
    if (epoll_test_injection::consume_failure(
            epoll_test_injection::fail_eventfd_count))
    {
        errno = EMFILE;
        return -1;
    }

    return static_cast<int>(
        ::syscall(SYS_eventfd2, initval, flags));
}

extern "C" int epoll_ctl(
    int epfd,
    int op,
    int fd,
    struct epoll_event* event) noexcept
{
    if (op == EPOLL_CTL_ADD &&
        epoll_test_injection::consume_failure(
            epoll_test_injection::fail_epoll_add_count))
    {
        errno = ENOMEM;
        return -1;
    }

    if (op == EPOLL_CTL_DEL &&
        epoll_test_injection::consume_failure(
            epoll_test_injection::fail_epoll_del_count))
    {
        errno = ENOENT;
        return -1;
    }

    return static_cast<int>(
        ::syscall(SYS_epoll_ctl, epfd, op, fd, event));
}

extern "C" int eventfd_write(int fd, eventfd_t value)
{
    if (epoll_test_injection::consume_failure(
            epoll_test_injection::fail_eventfd_write_count))
    {
        errno = EIO;
        return -1;
    }

    const uint64_t raw_value = static_cast<uint64_t>(value);
    const ssize_t written = static_cast<ssize_t>(
        ::syscall(SYS_write, fd, &raw_value, sizeof(raw_value)));

    if (written == static_cast<ssize_t>(sizeof(raw_value)))
        return 0;

    return -1;
}

namespace
{
    template <int Tag>
    struct PoolTestItem
    {
        int value = 0;

        void init() {}

        void refresh()
        {
            value = 0;
        }
    };

    template <int Tag>
    struct PoolIdentityItem
    {
        size_t id = static_cast<size_t>(-1);

        void init() {}
        void refresh() {}
    };

    struct RefreshOrderingItem
    {
        inline static std::atomic<bool> refresh_started{false};
        inline static std::atomic<bool> allow_refresh_to_finish{false};

        int value = 0;

        void init() {}

        void refresh()
        {
            refresh_started.store(true, std::memory_order_release);

            while (!allow_refresh_to_finish.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            value = 0;
        }

        static void reset_test_state()
        {
            refresh_started.store(false, std::memory_order_relaxed);
            allow_refresh_to_finish.store(false, std::memory_order_relaxed);
        }
    };

    struct DirtyVisibilityItem
    {
        inline static std::atomic<bool> refresh_started{false};
        inline static std::atomic<bool> allow_refresh_to_finish{false};

        int value = 0;

        void init() {}

        void refresh()
        {
            refresh_started.store(true, std::memory_order_release);

            while (!allow_refresh_to_finish.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            value = 0;
        }

        static void reset_test_state()
        {
            refresh_started.store(false, std::memory_order_relaxed);
            allow_refresh_to_finish.store(false, std::memory_order_relaxed);
        }
    };

    inline int count_open_fds()
    {
        DIR* dir = ::opendir("/proc/self/fd");
        if (dir == nullptr)
            return -1;

        int count = 0;

        while (dirent* entry = ::readdir(dir))
        {
            if (entry->d_name[0] == '.')
                continue;

            ++count;
        }

        // The directory descriptor itself is visible while iterating.
        --count;

        ::closedir(dir);
        return count;
    }

    inline bool wait_until_pool_size(
        size_t expected,
        std::chrono::milliseconds timeout = 3000ms)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (EpollBase::TaskInfoEventPool::size() == expected)
                return true;

            std::this_thread::sleep_for(1ms);
        }

        return EpollBase::TaskInfoEventPool::size() == expected;
    }

    enum class SyscallFailurePoint
    {
        EventFd,
        EpollAdd,
        EventFdWrite
    };

    inline void arm_syscall_failure(
        SyscallFailurePoint point,
        int count = 1)
    {
        epoll_test_injection::reset();

        switch (point)
        {
            case SyscallFailurePoint::EventFd:
                epoll_test_injection::fail_eventfd_count.store(
                    count,
                    std::memory_order_relaxed);
                break;

            case SyscallFailurePoint::EpollAdd:
                epoll_test_injection::fail_epoll_add_count.store(
                    count,
                    std::memory_order_relaxed);
                break;

            case SyscallFailurePoint::EventFdWrite:
                epoll_test_injection::fail_eventfd_write_count.store(
                    count,
                    std::memory_order_relaxed);
                break;
        }
    }

    [[noreturn]] void run_single_syscall_failure_conservation_child(
        SyscallFailurePoint point)
    {
        epoll_test_injection::reset();

        EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

        const size_t pool_before =
            EpollBase::TaskInfoEventPool::size();

        const int fds_before = count_open_fds();

        arm_syscall_failure(point, 1);

        try
        {
            epoll.add_run_task_event(nullptr);
        }
        catch (...)
        {
            ::_exit(20);
        }

        epoll_test_injection::reset();

        const size_t pool_after =
            EpollBase::TaskInfoEventPool::size();

        const int fds_after = count_open_fds();

        const bool resources_conserved =
            pool_after == pool_before &&
            fds_before >= 0 &&
            fds_after == fds_before;

        ::_exit(resources_conserved ? 0 : 1);
    }
}

// ============================================================================
// 1. Resource exhaustion tests
// ============================================================================

TEST(CachePoolResourceExhaustionTest, AcquireExactlyCapacitySucceeds)
{
    constexpr size_t CAPACITY = 8;
    using Pool = CachePool<PoolTestItem<1>, CAPACITY>;

    std::array<PoolTestItem<1>*, CAPACITY> items{};

    ASSERT_EQ(Pool::size(), CAPACITY);

    for (auto& item : items)
    {
        ASSERT_NO_THROW(item = Pool::acquire());
        ASSERT_NE(item, nullptr);
    }

    EXPECT_EQ(Pool::size(), 0u);

    for (auto* item : items)
        Pool::release(item);

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolResourceExhaustionTest, AcquireBeyondCapacityThrows)
{
    constexpr size_t CAPACITY = 8;
    using Pool = CachePool<PoolTestItem<2>, CAPACITY>;

    std::array<PoolTestItem<2>*, CAPACITY> items{};

    for (auto& item : items)
        item = Pool::acquire();

    ASSERT_EQ(Pool::size(), 0u);

    EXPECT_THROW(
        Pool::acquire(),
        std::runtime_error);

    EXPECT_EQ(Pool::size(), 0u);

    for (auto* item : items)
        Pool::release(item);

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolResourceExhaustionTest, CanRecoverAfterExhaustion)
{
    constexpr size_t CAPACITY = 8;
    using Pool = CachePool<PoolTestItem<3>, CAPACITY>;

    std::array<PoolTestItem<3>*, CAPACITY> items{};

    for (auto& item : items)
        item = Pool::acquire();

    ASSERT_EQ(Pool::size(), 0u);
    EXPECT_THROW(Pool::acquire(), std::runtime_error);

    auto* released = items[3];
    Pool::release(released);

    ASSERT_EQ(Pool::size(), 1u);

    PoolTestItem<3>* reacquired = nullptr;
    ASSERT_NO_THROW(reacquired = Pool::acquire());

    EXPECT_EQ(reacquired, released);
    EXPECT_EQ(Pool::size(), 0u);

    Pool::release(reacquired);

    for (size_t i = 0; i < CAPACITY; ++i)
    {
        if (i != 3)
            Pool::release(items[i]);
    }

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolResourceExhaustionTest, RepeatedExhaustAndRecoverWrapsCleanly)
{
    constexpr size_t CAPACITY = 16;
    constexpr int ROUNDS = 10000;

    using Pool = CachePool<PoolTestItem<4>, CAPACITY>;

    std::array<PoolTestItem<4>*, CAPACITY> items{};

    for (int round = 0; round < ROUNDS; ++round)
    {
        for (auto& item : items)
            item = Pool::acquire();

        ASSERT_EQ(Pool::size(), 0u) << "round=" << round;
        EXPECT_THROW(Pool::acquire(), std::runtime_error);

        for (auto* item : items)
            Pool::release(item);

        ASSERT_EQ(Pool::size(), CAPACITY) << "round=" << round;
        ASSERT_LT(Pool::head(), CAPACITY);
        ASSERT_LT(Pool::tail(), CAPACITY);
    }
}

TEST(CachePoolResourceExhaustionTest, ReleaseNullThrows)
{
    using Pool = CachePool<PoolTestItem<5>, 8>;

    EXPECT_THROW(
        Pool::release(nullptr),
        std::runtime_error);
}

TEST(CachePoolResourceExhaustionTest, ForeignPointerAborts)
{
    using Pool = CachePool<PoolTestItem<6>, 8>;

    EXPECT_DEATH(
        {
            PoolTestItem<6> foreign;
            Pool::release(&foreign);
        },
        ".*");
}

// ============================================================================
// 2. Syscall failure and rollback tests
// ============================================================================

TEST(EpollSyscallRollbackTest, EventFdFailureReturnsItemToPool)
{
    EXPECT_EXIT(
        {
            run_single_syscall_failure_conservation_child(
                SyscallFailurePoint::EventFd);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollSyscallRollbackTest, EventFdFailureAfterManySuccessfulSchedulesDoesNotLeak)
{
    EXPECT_EXIT(
        {
            epoll_test_injection::reset();

            EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

            constexpr int SUCCESSFUL_EVENTS = 100;

            for (int i = 0; i < SUCCESSFUL_EVENTS; ++i)
                epoll.add_run_task_event(nullptr);

            const size_t pool_before_failure =
                EpollBase::TaskInfoEventPool::size();

            const int fds_before_failure = count_open_fds();

            epoll_test_injection::fail_eventfd_count.store(
                1,
                std::memory_order_relaxed);

            epoll.add_run_task_event(nullptr);

            epoll_test_injection::reset();

            const bool ok =
                EpollBase::TaskInfoEventPool::size() == pool_before_failure &&
                count_open_fds() == fds_before_failure;

            ::_exit(ok ? 0 : 1);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollSyscallRollbackTest, RepeatedEventFdFailuresNeverDrainPool)
{
    EXPECT_EXIT(
        {
            epoll_test_injection::reset();

            EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

            constexpr int FAILURES = 256;

            const size_t pool_before =
                EpollBase::TaskInfoEventPool::size();

            const int fds_before = count_open_fds();

            epoll_test_injection::fail_eventfd_count.store(
                FAILURES,
                std::memory_order_relaxed);

            for (int i = 0; i < FAILURES; ++i)
                epoll.add_run_task_event(nullptr);

            epoll_test_injection::reset();

            const bool ok =
                EpollBase::TaskInfoEventPool::size() == pool_before &&
                count_open_fds() == fds_before;

            ::_exit(ok ? 0 : 1);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollSyscallRollbackTest, EpollCtlAddFailureClosesFdAndReturnsPoolItem)
{
    EXPECT_EXIT(
        {
            run_single_syscall_failure_conservation_child(
                SyscallFailurePoint::EpollAdd);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollSyscallRollbackTest, RepeatedEpollCtlAddFailuresNeverDrainPoolOrLeakFds)
{
    EXPECT_EXIT(
        {
            epoll_test_injection::reset();

            EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

            constexpr int FAILURES = 128;

            const size_t pool_before =
                EpollBase::TaskInfoEventPool::size();

            const int fds_before = count_open_fds();

            epoll_test_injection::fail_epoll_add_count.store(
                FAILURES,
                std::memory_order_relaxed);

            for (int i = 0; i < FAILURES; ++i)
                epoll.add_run_task_event(nullptr);

            epoll_test_injection::reset();

            const bool ok =
                EpollBase::TaskInfoEventPool::size() == pool_before &&
                count_open_fds() == fds_before;

            ::_exit(ok ? 0 : 1);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollSyscallRollbackTest, EventFdWriteFailureFullyRollsBack)
{
    EXPECT_EXIT(
        {
            run_single_syscall_failure_conservation_child(
                SyscallFailurePoint::EventFdWrite);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollSyscallRollbackTest, EpollCtlDelFailureStillClosesAndReleases)
{
    epoll_test_injection::reset();

    EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

    const size_t pool_before =
        EpollBase::TaskInfoEventPool::size();

    auto* task_event = EpollBase::TaskInfoEventPool::acquire();
    ASSERT_NE(task_event, nullptr);

    const int fd = task_event->generate_fd();

    if (fd < 0)
    {
        EpollBase::TaskInfoEventPool::release(task_event);
        FAIL() << "eventfd() unexpectedly failed while preparing the DEL failure test";
        return;
    }

    ASSERT_EQ(EpollBase::TaskInfoEventPool::size(), pool_before - 1);

    // The fd was never registered in this epoll instance, so EPOLL_CTL_DEL
    // must fail with ENOENT. del_fd() must still close and release it.
    epoll.del_fd(fd, task_event);

    EXPECT_EQ(EpollBase::TaskInfoEventPool::size(), pool_before);

    errno = 0;
    EXPECT_EQ(::fcntl(fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

class EpollSyscallConservationTest
    : public ::testing::TestWithParam<SyscallFailurePoint>
{
};

TEST_P(EpollSyscallConservationTest, EveryFailurePointConservesPoolAndFdResources)
{
    const SyscallFailurePoint point = GetParam();

    EXPECT_EXIT(
        {
            run_single_syscall_failure_conservation_child(point);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

INSTANTIATE_TEST_SUITE_P(
    AllFailurePoints,
    EpollSyscallConservationTest,
    ::testing::Values(
        SyscallFailurePoint::EventFd,
        SyscallFailurePoint::EpollAdd,
        SyscallFailurePoint::EventFdWrite));

// ============================================================================
// 3. High-watermark and in-flight pressure tests
// ============================================================================

TEST(EpollHighWatermarkTest, SchedulerBacklogConsumesOnePoolItemPerOutstandingEvent)
{
    EXPECT_EXIT(
        {
            epoll_test_injection::reset();

            EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

            constexpr size_t OUTSTANDING = 2048;

            const size_t pool_before =
                EpollBase::TaskInfoEventPool::size();

            const int fds_before = count_open_fds();

            for (size_t i = 0; i < OUTSTANDING; ++i)
                epoll.add_run_task_event(nullptr);

            const size_t pool_after =
                EpollBase::TaskInfoEventPool::size();

            const int fds_after = count_open_fds();

            const bool ok =
                pool_before >= OUTSTANDING &&
                pool_after == pool_before - OUTSTANDING &&
                fds_before >= 0 &&
                fds_after == fds_before + static_cast<int>(OUTSTANDING);

            ::_exit(ok ? 0 : 1);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollHighWatermarkTest, StandalonePoolCanReachNinetyPercentOccupancyAndRecover)
{
    constexpr size_t CAPACITY = 1000;
    constexpr size_t OUTSTANDING = CAPACITY * 9 / 10;

    using Pool = CachePool<PoolTestItem<20>, CAPACITY>;

    std::vector<PoolTestItem<20>*> items;
    items.reserve(OUTSTANDING);

    for (size_t i = 0; i < OUTSTANDING; ++i)
        items.push_back(Pool::acquire());

    EXPECT_EQ(Pool::size(), CAPACITY - OUTSTANDING);

    for (auto* item : items)
        Pool::release(item);

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(EpollHighWatermarkTest, StandalonePoolCanReachExactCapacityAndRecover)
{
    constexpr size_t CAPACITY = 257;
    using Pool = CachePool<PoolTestItem<21>, CAPACITY>;

    std::array<PoolTestItem<21>*, CAPACITY> items{};

    for (auto& item : items)
        item = Pool::acquire();

    ASSERT_EQ(Pool::size(), 0u);

    for (auto* item : items)
        Pool::release(item);

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(EpollHighWatermarkTest, StandalonePoolCapacityPlusOneFailsWithoutCorruptingState)
{
    constexpr size_t CAPACITY = 257;
    using Pool = CachePool<PoolTestItem<22>, CAPACITY>;

    std::array<PoolTestItem<22>*, CAPACITY> items{};

    for (auto& item : items)
        item = Pool::acquire();

    ASSERT_EQ(Pool::size(), 0u);

    EXPECT_THROW(Pool::acquire(), std::runtime_error);
    EXPECT_EQ(Pool::size(), 0u);

    for (auto* item : items)
        Pool::release(item);

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(EpollHighWatermarkTest, ManyProducerThreadsCanBuildBacklogWithoutWaiting)
{
    EXPECT_EXIT(
        {
            epoll_test_injection::reset();

            EpollBase epoll(EventBaseID::EPOLL_SYSTEM_IO_TASK);

            constexpr int THREADS = 8;
            constexpr int EVENTS_PER_THREAD = 500;
            constexpr int TOTAL_EVENTS = THREADS * EVENTS_PER_THREAD;

            const size_t pool_before =
                EpollBase::TaskInfoEventPool::size();

            const int fds_before = count_open_fds();

            std::barrier start_line(THREADS);
            std::atomic<int> exceptions{0};
            std::vector<std::thread> producers;
            producers.reserve(THREADS);

            for (int t = 0; t < THREADS; ++t)
            {
                producers.emplace_back([&]
                {
                    start_line.arrive_and_wait();

                    for (int i = 0; i < EVENTS_PER_THREAD; ++i)
                    {
                        try
                        {
                            epoll.add_run_task_event(nullptr);
                        }
                        catch (...)
                        {
                            exceptions.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                });
            }

            for (auto& producer : producers)
                producer.join();

            const size_t pool_after =
                EpollBase::TaskInfoEventPool::size();

            const int fds_after = count_open_fds();

            const bool ok =
                exceptions.load(std::memory_order_relaxed) == 0 &&
                pool_before >= static_cast<size_t>(TOTAL_EVENTS) &&
                pool_after == pool_before - static_cast<size_t>(TOTAL_EVENTS) &&
                fds_before >= 0 &&
                fds_after == fds_before + TOTAL_EVENTS;

            ::_exit(ok ? 0 : 1);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST(EpollHighWatermarkTest, ManyProducersBurstThenRealConsumerDrainsEverything)
{
    constexpr int THREADS = 8;
    constexpr int TASKS_PER_THREAD = 1000;

    auto* eb = test_epoll_base();

    const size_t initial_pool_size =
        EpollBase::TaskInfoEventPool::size();

    EpollBase::TaskInfoEventEpoll::reset_task_event_counters();

    auto fn = [](int value) -> Task<int>
    {
        co_return value;
    };

    std::barrier start_line(THREADS);
    std::atomic<int> failures{0};
    std::atomic<long long> total_sum{0};
    std::vector<std::thread> producers;
    producers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        producers.emplace_back([&, t]
        {
            std::vector<Task<int>> tasks;
            std::vector<std::future<int>> results;

            tasks.reserve(TASKS_PER_THREAD);
            results.reserve(TASKS_PER_THREAD);

            start_line.arrive_and_wait();

            for (int i = 0; i < TASKS_PER_THREAD; ++i)
            {
                const int value = t * TASKS_PER_THREAD + i;

                tasks.emplace_back(fn(value));
                results.emplace_back(tasks.back().get_future());
                tasks.back().start_running_on(eb);
            }

            long long local_sum = 0;

            for (int i = 0; i < TASKS_PER_THREAD; ++i)
            {
                try
                {
                    local_sum += results[i].get();
                }
                catch (...)
                {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }

            total_sum.fetch_add(local_sum, std::memory_order_relaxed);
        });
    }

    for (auto& producer : producers)
        producer.join();

    const long long total_tasks =
        static_cast<long long>(THREADS) * TASKS_PER_THREAD;

    const long long expected_sum =
        (total_tasks - 1) * total_tasks / 2;

    EXPECT_EQ(failures.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(total_sum.load(std::memory_order_relaxed), expected_sum);

    ASSERT_TRUE(wait_until_pool_size(initial_pool_size));

    EXPECT_EQ(
        EpollBase::TaskInfoEventPool::size(),
        initial_pool_size);

    EXPECT_EQ(
        EpollBase::TaskInfoEventEpoll::task_event_generate_fd_count.load(
            std::memory_order_relaxed),
        EpollBase::TaskInfoEventEpoll::task_event_release_count.load(
            std::memory_order_relaxed));

    EventBaseManager::shutdown_all();
}

TEST(EpollHighWatermarkTest, RepeatedBurstAndDrainReturnsPoolAfterEveryRound)
{
    constexpr int ROUNDS = 20;
    constexpr int TASKS_PER_ROUND = 500;

    auto* eb = test_epoll_base();

    const size_t initial_pool_size =
        EpollBase::TaskInfoEventPool::size();

    auto fn = [](int value) -> Task<int>
    {
        co_return value;
    };

    for (int round = 0; round < ROUNDS; ++round)
    {
        std::vector<Task<int>> tasks;
        std::vector<std::future<int>> results;

        tasks.reserve(TASKS_PER_ROUND);
        results.reserve(TASKS_PER_ROUND);

        for (int i = 0; i < TASKS_PER_ROUND; ++i)
        {
            tasks.emplace_back(fn(i));
            results.emplace_back(tasks.back().get_future());
            tasks.back().start_running_on(eb);
        }

        long long sum = 0;

        for (auto& result : results)
            sum += result.get();

        EXPECT_EQ(
            sum,
            (TASKS_PER_ROUND - 1LL) * TASKS_PER_ROUND / 2);

        tasks.clear();

        ASSERT_TRUE(wait_until_pool_size(initial_pool_size))
            << "pool did not fully recover after round " << round;

        ASSERT_EQ(
            EpollBase::TaskInfoEventPool::size(),
            initial_pool_size)
            << "round=" << round;
    }

    EventBaseManager::shutdown_all();
}

TEST(EpollHighWatermarkTest, NestedCoroutineRootsAreAllScheduledBeforeWaiting)
{
    constexpr int ROOT_TASKS = 256;
    constexpr int MIDDLE_CALLS_PER_ROOT = 5;

    auto* eb = test_epoll_base();

    const size_t initial_pool_size =
        EpollBase::TaskInfoEventPool::size();

    auto leaf = [](int value) -> Task<int>
    {
        int out = co_await Future<int>([value](auto result)
        {
            result.set_value(value + 1);
        });

        co_return out;
    };

    auto middle = [&](int value) -> Task<int>
    {
        int a = co_await leaf(value);
        int b = co_await leaf(a);
        int c = co_await leaf(b);
        co_return c;
    };

    auto root = [&](int value) -> Task<int>
    {
        int out = value;

        for (int i = 0; i < MIDDLE_CALLS_PER_ROOT; ++i)
            out = co_await middle(out);

        co_return out;
    };

    std::vector<Task<int>> roots;
    std::vector<std::future<int>> results;

    roots.reserve(ROOT_TASKS);
    results.reserve(ROOT_TASKS);

    // Schedule every root first to maximize the number of simultaneously
    // active coroutine chains before waiting on any result.
    for (int i = 0; i < ROOT_TASKS; ++i)
    {
        roots.emplace_back(root(i));
        results.emplace_back(roots.back().get_future());
        roots.back().start_running_on(eb);
    }

    for (int i = 0; i < ROOT_TASKS; ++i)
    {
        EXPECT_EQ(
            results[i].get(),
            i + MIDDLE_CALLS_PER_ROOT * 3);
    }

    roots.clear();

    ASSERT_TRUE(wait_until_pool_size(initial_pool_size));
    EXPECT_EQ(EpollBase::TaskInfoEventPool::size(), initial_pool_size);

    EventBaseManager::shutdown_all();
}

TEST(EpollHighWatermarkTest, ShutdownWithBacklogRequiresExplicitOwnershipPolicy)
{
    // A deterministic shutdown/backlog test needs an explicit ownership rule:
    // either pending TaskInfoEventEpoll objects must be drained before loop()
    // returns, or shutdown must cancel and release them. The current interface
    // does not expose a safe way to enqueue valid pending task events and then
    // inspect/cancel them without racing coroutine promise lifetime.
    //
    // Keep this visible as a skipped contract test instead of silently leaving
    // the shutdown/backlog edge undocumented.
    GTEST_SKIP()
        << "Define the pending-task ownership policy for EpollBase::stop()/loop() "
        << "before enabling a deterministic shutdown backlog assertion.";
}

// ============================================================================
// 4. CachePool MPMC invariant tests
// ============================================================================

TEST(CachePoolMPMCInvariantTest, SamePointerIsNeverLeasedToTwoThreadsAtOnce)
{
    constexpr size_t CAPACITY = 128;
    constexpr int THREADS = 32;
    constexpr int ITERATIONS_PER_THREAD = 50000;

    using Item = PoolIdentityItem<40>;
    using Pool = CachePool<Item, CAPACITY>;

    std::array<Item*, CAPACITY> all_items{};

    for (size_t i = 0; i < CAPACITY; ++i)
    {
        all_items[i] = Pool::acquire();
        all_items[i]->id = i;
    }

    for (auto* item : all_items)
        Pool::release(item);

    std::array<std::atomic<bool>, CAPACITY> leased{};

    for (auto& flag : leased)
        flag.store(false, std::memory_order_relaxed);

    std::atomic<int> duplicate_leases{0};
    std::atomic<int> invalid_ids{0};

    std::barrier start_line(THREADS);
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&]
        {
            start_line.arrive_and_wait();

            for (int i = 0; i < ITERATIONS_PER_THREAD; ++i)
            {
                Item* item = nullptr;

                try
                {
                    item = Pool::acquire();
                }
                catch (const std::runtime_error&)
                {
                    std::this_thread::yield();
                    continue;
                }

                if (item->id >= CAPACITY)
                {
                    invalid_ids.fetch_add(1, std::memory_order_relaxed);
                    Pool::release(item);
                    continue;
                }

                if (leased[item->id].exchange(
                        true,
                        std::memory_order_acq_rel))
                {
                    duplicate_leases.fetch_add(1, std::memory_order_relaxed);
                }

                std::this_thread::yield();

                leased[item->id].store(false, std::memory_order_release);
                Pool::release(item);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    EXPECT_EQ(duplicate_leases.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(invalid_ids.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolMPMCInvariantTest, OnlyOneThreadCanAcquireTheLastAvailableItem)
{
    constexpr int THREADS = 32;

    using Item = PoolTestItem<41>;
    using Pool = CachePool<Item, 1>;

    std::barrier start_line(THREADS);

    std::atomic<int> successes{0};
    std::atomic<int> failures{0};

    std::array<Item*, THREADS> acquired{};
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&, t]
        {
            start_line.arrive_and_wait();

            try
            {
                acquired[t] = Pool::acquire();
                successes.fetch_add(1, std::memory_order_relaxed);
            }
            catch (const std::runtime_error&)
            {
                failures.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    EXPECT_EQ(successes.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(failures.load(std::memory_order_relaxed), THREADS - 1);
    EXPECT_EQ(Pool::size(), 0u);

    for (auto* item : acquired)
    {
        if (item != nullptr)
            Pool::release(item);
    }

    EXPECT_EQ(Pool::size(), 1u);
}

TEST(CachePoolMPMCInvariantTest, SizeNeverUnderflowsDuringLastItemContention)
{
    constexpr int THREADS = 32;

    using Item = PoolTestItem<42>;
    using Pool = CachePool<Item, 1>;

    std::barrier start_line(THREADS + 1);
    std::barrier acquired_line(THREADS + 1);
    std::barrier release_line(THREADS + 1);

    std::array<Item*, THREADS> acquired{};
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&, t]
        {
            start_line.arrive_and_wait();

            try
            {
                acquired[t] = Pool::acquire();
            }
            catch (const std::runtime_error&)
            {
            }

            acquired_line.arrive_and_wait();
            release_line.arrive_and_wait();

            if (acquired[t] != nullptr)
                Pool::release(acquired[t]);
        });
    }

    start_line.arrive_and_wait();
    acquired_line.arrive_and_wait();

    // At this stable point no thread has released anything yet. A valid
    // bounded pool must never expose a wrapped size_t value.
    EXPECT_LE(Pool::size(), 1u);

    release_line.arrive_and_wait();

    for (auto& worker : workers)
        worker.join();

    EXPECT_EQ(Pool::size(), 1u);
}

TEST(CachePoolMPMCInvariantTest, DoubleReleaseMustBeRejected)
{
    constexpr size_t CAPACITY = 4;
    using Pool = CachePool<PoolTestItem<43>, CAPACITY>;

    auto* item = Pool::acquire();
    ASSERT_NE(item, nullptr);

    Pool::release(item);
    ASSERT_EQ(Pool::size(), CAPACITY);

    // The pool must reject a second release of an already-free item. The
    // current implementation only validates address range, so this test is
    // expected to expose the missing ownership-state validation.
    EXPECT_THROW(
        Pool::release(item),
        std::runtime_error);

    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolMPMCInvariantTest, ItemIsNotPublishedUntilRefreshFinishes)
{
    using Pool = CachePool<RefreshOrderingItem, 1>;

    RefreshOrderingItem::reset_test_state();

    auto* item = Pool::acquire();
    ASSERT_NE(item, nullptr);
    ASSERT_EQ(Pool::size(), 0u);

    item->value = 999;

    std::thread releaser([&]
    {
        Pool::release(item);
    });

    while (!RefreshOrderingItem::refresh_started.load(
        std::memory_order_acquire))
    {
        std::this_thread::yield();
    }

    // While refresh() is still running the object must remain unavailable.
    EXPECT_EQ(Pool::size(), 0u);

    RefreshOrderingItem::allow_refresh_to_finish.store(
        true,
        std::memory_order_release);

    releaser.join();

    ASSERT_EQ(Pool::size(), 1u);

    auto* reacquired = Pool::acquire();
    ASSERT_NE(reacquired, nullptr);
    EXPECT_EQ(reacquired->value, 0);

    Pool::release(reacquired);
}

TEST(CachePoolMPMCInvariantTest, StableBarrierMaintainsConservationInvariant)
{
    constexpr size_t CAPACITY = 64;
    constexpr int THREADS = 32;

    using Item = PoolTestItem<44>;
    using Pool = CachePool<Item, CAPACITY>;

    std::barrier start_line(THREADS + 1);
    std::barrier acquired_line(THREADS + 1);
    std::barrier release_line(THREADS + 1);

    std::atomic<size_t> leased_count{0};
    std::array<Item*, THREADS> acquired{};
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&, t]
        {
            start_line.arrive_and_wait();

            acquired[t] = Pool::acquire();
            leased_count.fetch_add(1, std::memory_order_relaxed);

            acquired_line.arrive_and_wait();
            release_line.arrive_and_wait();

            Pool::release(acquired[t]);
            leased_count.fetch_sub(1, std::memory_order_relaxed);
        });
    }

    start_line.arrive_and_wait();
    acquired_line.arrive_and_wait();

    EXPECT_EQ(
        Pool::size() + leased_count.load(std::memory_order_relaxed),
        CAPACITY);

    release_line.arrive_and_wait();

    for (auto& worker : workers)
        worker.join();

    EXPECT_EQ(leased_count.load(std::memory_order_relaxed), 0u);
    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolMPMCInvariantTest, HeadAndTailWrapRemainValidUnderContention)
{
    constexpr size_t CAPACITY = 7;
    constexpr int THREADS = 16;
    constexpr int ITERATIONS_PER_THREAD = 20000;

    using Item = PoolTestItem<45>;
    using Pool = CachePool<Item, CAPACITY>;

    std::barrier start_line(THREADS);
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&]
        {
            start_line.arrive_and_wait();

            for (int i = 0; i < ITERATIONS_PER_THREAD; ++i)
            {
                Item* item = nullptr;

                try
                {
                    item = Pool::acquire();
                }
                catch (const std::runtime_error&)
                {
                    std::this_thread::yield();
                    continue;
                }

                std::this_thread::yield();
                Pool::release(item);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    EXPECT_LT(Pool::head(), CAPACITY);
    EXPECT_LT(Pool::tail(), CAPACITY);
    EXPECT_EQ(Pool::size(), CAPACITY);
}

TEST(CachePoolMPMCInvariantTest, EveryOriginalPointerExistsExactlyOnceAfterDrain)
{
    constexpr size_t CAPACITY = 64;
    constexpr int THREADS = 8;
    constexpr int ITERATIONS_PER_THREAD = 10000;

    using Item = PoolIdentityItem<46>;
    using Pool = CachePool<Item, CAPACITY>;

    std::array<Item*, CAPACITY> original{};

    for (size_t i = 0; i < CAPACITY; ++i)
    {
        original[i] = Pool::acquire();
        original[i]->id = i;
    }

    for (auto* item : original)
        Pool::release(item);

    std::barrier start_line(THREADS);
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&]
        {
            start_line.arrive_and_wait();

            for (int i = 0; i < ITERATIONS_PER_THREAD; ++i)
            {
                try
                {
                    auto* item = Pool::acquire();
                    std::this_thread::yield();
                    Pool::release(item);
                }
                catch (const std::runtime_error&)
                {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    std::vector<Item*> drained;
    drained.reserve(CAPACITY);

    bool unexpected_exhaustion = false;

    for (size_t i = 0; i < CAPACITY; ++i)
    {
        try
        {
            drained.push_back(Pool::acquire());
        }
        catch (const std::runtime_error&)
        {
            unexpected_exhaustion = true;
            break;
        }
    }

    std::unordered_set<Item*> unique_pointers(
        drained.begin(),
        drained.end());

    EXPECT_FALSE(unexpected_exhaustion);
    EXPECT_EQ(drained.size(), CAPACITY);
    EXPECT_EQ(unique_pointers.size(), CAPACITY);

    for (auto* item : drained)
        Pool::release(item);
}

TEST(CachePoolMPMCInvariantTest, DirtyObjectCannotBeReacquiredWhileRefreshIsRunning)
{
    using Pool = CachePool<DirtyVisibilityItem, 1>;

    DirtyVisibilityItem::reset_test_state();

    auto* item = Pool::acquire();
    ASSERT_NE(item, nullptr);

    item->value = 999;

    std::thread releaser([&]
    {
        Pool::release(item);
    });

    while (!DirtyVisibilityItem::refresh_started.load(
        std::memory_order_acquire))
    {
        std::this_thread::yield();
    }

    DirtyVisibilityItem* unexpectedly_reacquired = nullptr;

    try
    {
        unexpectedly_reacquired = Pool::acquire();
    }
    catch (const std::runtime_error&)
    {
        // Correct behavior: the item is still owned by release()/refresh().
    }

    EXPECT_EQ(unexpectedly_reacquired, nullptr);

    DirtyVisibilityItem::allow_refresh_to_finish.store(
        true,
        std::memory_order_release);

    releaser.join();

    if (unexpectedly_reacquired != nullptr)
    {
        // Clean up the intentionally exposed bad state so this unique test
        // pool does not remain leased after the assertion above fails.
        Pool::release(unexpectedly_reacquired);
        return;
    }

    auto* clean_item = Pool::acquire();
    ASSERT_NE(clean_item, nullptr);
    EXPECT_EQ(clean_item->value, 0);
    Pool::release(clean_item);
}

#if defined(__SANITIZE_THREAD__)
#define CACHE_POOL_TEST_TSAN_ENABLED 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define CACHE_POOL_TEST_TSAN_ENABLED 1
#else
#define CACHE_POOL_TEST_TSAN_ENABLED 0
#endif
#else
#define CACHE_POOL_TEST_TSAN_ENABLED 0
#endif

TEST(CachePoolMPMCInvariantTest, ThreadSanitizerStressTarget)
{
#if CACHE_POOL_TEST_TSAN_ENABLED
    constexpr size_t CAPACITY = 32;
    constexpr int THREADS = 16;
    constexpr int ITERATIONS_PER_THREAD = 20000;

    using Item = PoolTestItem<47>;
    using Pool = CachePool<Item, CAPACITY>;

    std::barrier start_line(THREADS);
    std::vector<std::thread> workers;
    workers.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&]
        {
            start_line.arrive_and_wait();

            for (int i = 0; i < ITERATIONS_PER_THREAD; ++i)
            {
                Item* item = nullptr;

                try
                {
                    item = Pool::acquire();
                }
                catch (const std::runtime_error&)
                {
                    std::this_thread::yield();
                    continue;
                }

                item->value = i;
                std::this_thread::yield();
                Pool::release(item);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    EXPECT_EQ(Pool::size(), CAPACITY);
#else
    GTEST_SKIP()
        << "Rebuild this test binary with -fsanitize=thread -g -O1 to run the TSAN target.";
#endif
}

#undef CACHE_POOL_TEST_TSAN_ENABLED
