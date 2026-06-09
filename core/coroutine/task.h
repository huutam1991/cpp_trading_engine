#pragma once

#include <atomic>
#include <cstddef>
#include <future>

#include "base_task.h"

template<class T>
struct Task : public BaseTask
{
    struct promise_type : public BaseTask::promise_type
    {
#ifdef TEST_MODE_ONLY
        inline static std::atomic<int64_t> alloc_count{0};
        inline static std::atomic<int64_t> free_count{0};

        static void* operator new(std::size_t size)
        {
            alloc_count.fetch_add(1, std::memory_order_relaxed);
            return ::operator new(size);
        }

        static void operator delete(void* ptr, std::size_t) noexcept
        {
            free_count.fetch_add(1, std::memory_order_relaxed);
            ::operator delete(ptr);
        }

        static void reset_frame_counters()
        {
            alloc_count.store(0, std::memory_order_relaxed);
            free_count.store(0, std::memory_order_relaxed);
        }

        static int64_t frame_alloc_count()
        {
            return alloc_count.load(std::memory_order_relaxed);
        }

        static int64_t frame_free_count()
        {
            return free_count.load(std::memory_order_relaxed);
        }
#endif

        Task get_return_object()
        {
            std::coroutine_handle<promise_type> h = std::coroutine_handle<promise_type>::from_promise(*this);
            this->handle = h;
            return Task{this};
        }

        void return_value(T v)
        {
            if (this->task_value != nullptr)
            {
                this->task_value->set_value(std::move(v));
            }
            else
            {
                value = std::move(v);
            }

            if (this->m_suspending_promise != nullptr)
            {
                this->m_event_base->add_set_suspend_value_event(this->m_suspending_promise);
            }
        }

        // Promise value
        T value;
        std::shared_ptr<std::promise<T>> task_value = nullptr;
    };

    Task(std::nullptr_t) : BaseTask(nullptr) {}
    Task(promise_type* promise) : BaseTask(promise) {}
    Task() = default;

    // Only allow move constructor and move assignment
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;

    // Delete copy constructor and copy assignment
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task& operator=(std::nullptr_t null)
    {
        BaseTask::operator=(null);
        return *this;
    }

    bool operator==(std::nullptr_t null) const
    {
        return BaseTask::operator==(null);
    }

    T await_resume()
    {
        Task<T>::promise_type* promise = (Task<T>::promise_type*)m_promise;

        if (promise->task_value != nullptr)
        {
            return promise->task_value->get_future().get();
        }
        else
        {
            return std::move(promise->value);
        }
    }

    inline std::future<T> start_running_on(EventBase* event_base)
    {
        Task<T>::promise_type* promise = (Task<T>::promise_type*)m_promise;
        promise->task_value = std::make_unique<std::promise<T>>();

        register_on(event_base);

        return promise->task_value->get_future();
    }
};

template<>
struct Task<void> : public BaseTask
{
    struct promise_type : public BaseTask::promise_type
    {

#ifdef TEST_MODE_ONLY
        inline static std::atomic<int64_t> alloc_count{0};
        inline static std::atomic<int64_t> free_count{0};

        static void* operator new(std::size_t size)
        {
            alloc_count.fetch_add(1, std::memory_order_relaxed);
            return ::operator new(size);
        }

        static void operator delete(void* ptr, std::size_t) noexcept
        {
            free_count.fetch_add(1, std::memory_order_relaxed);
            ::operator delete(ptr);
        }

        static void reset_frame_counters()
        {
            alloc_count.store(0, std::memory_order_relaxed);
            free_count.store(0, std::memory_order_relaxed);
        }

        static int64_t frame_alloc_count()
        {
            return alloc_count.load(std::memory_order_relaxed);
        }

        static int64_t frame_free_count()
        {
            return free_count.load(std::memory_order_relaxed);
        }
#endif

        Task get_return_object()
        {
            std::coroutine_handle<promise_type> h = std::coroutine_handle<promise_type>::from_promise(*this);
            this->handle = h;

            return Task{this};
        }

        void return_void()
        {
            if (this->task_value != nullptr)
            {
                this->task_value->set_value();
            }

            if (this->m_suspending_promise != nullptr)
            {
                this->m_event_base->add_set_suspend_value_event(this->m_suspending_promise);
            }
        }

        // Promise value
        std::unique_ptr<std::promise<void>> task_value = nullptr;
    };

    Task(std::nullptr_t) : BaseTask(nullptr) {}
    Task(promise_type* promise) : BaseTask(promise) {}
    Task() = default;

    // Only allow move constructor and move assignment
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;

    // Delete copy constructor and copy assignment
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task& operator=(std::nullptr_t null)
    {
        BaseTask::operator=(null);
        return *this;
    }

    bool operator==(std::nullptr_t null) const
    {
        return BaseTask::operator==(null);
    }

    void await_resume()
    {
        return;
    }

    inline std::future<void> start_running_on(EventBase* event_base)
    {
        Task<void>::promise_type* promise = (Task<void>::promise_type*)m_promise;
        promise->task_value = std::make_unique<std::promise<void>>();

        register_on(event_base);

        return promise->task_value->get_future();
    }
};