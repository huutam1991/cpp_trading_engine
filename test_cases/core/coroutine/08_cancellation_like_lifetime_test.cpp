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

TEST(CoroutineUsageCancellationLikeTest, DestroySuspendedTaskThenFutureCompletesLater)
{
    using FutureValue = typename Future<int>::FutureValue;

    std::shared_ptr<FutureValue*> saved = std::make_shared<FutureValue*>(nullptr);

    auto fn = [saved]() -> Task<int>
    {
        int v = co_await Future<int>([saved](auto* out)
        {
            *saved = out;
        });

        co_return v;
    };

    {
        auto task = fn();
        auto result = task.start_running_on(test_event_base());

        // Give the coroutine a chance to suspend on Future.
        std::this_thread::sleep_for(5ms);
    }

    if (*saved != nullptr)
    {
        (*saved)->set_value(42);
    }

    SUCCEED();
}

TEST(CoroutineUsageCancellationLikeTest, DestroyParentWhileChildMayStillRun)
{
    auto child = []() -> Task<int>
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

    auto parent = [&]() -> Task<int>
    {
        int v = co_await child();
        co_return v;
    };

    {
        auto task = parent();
        auto result = task.start_running_on(test_event_base());
        std::this_thread::sleep_for(1ms);
    }

    std::this_thread::sleep_for(20ms);
    SUCCEED();
}

TEST(CoroutineUsageCancellationLikeTest, RepeatedDestroyByScopeExitIsSafe)
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
