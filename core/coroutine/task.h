#pragma once

#include <atomic>
#include <cstddef>

#include "base_promise_type.h"
#include "base_task.h"

template<class T>
struct Task : public BaseTask<T>
{
    struct promise_type : public BaseTask<T>::promise_type
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
            return Task{this};
        }

        void return_value(T v)
        {
            this->task_value.set_value(v);
        }
    };

    Task(std::nullptr_t) : BaseTask<T>(nullptr) {}
    Task(promise_type* promise) : BaseTask<T>(promise) {}
    Task() = default;
    Task(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = default;

    T await_resume()
    {
        auto& promise = this->handle.promise();
        return static_cast<promise_type*>(&promise)->task_value.value;
    }
};

template<>
struct Task<void> : public BaseTask<void>
{
    struct promise_type : public BaseTask<void>::promise_type
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
            return Task{this};
        }

        void return_void()
        {
            this->task_value.set_value();
        }
    };

    Task(std::nullptr_t) : BaseTask<void>(nullptr) {}
    Task(promise_type* promise) : BaseTask<void>(promise) {}
    Task() = default;
    Task(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = default;

    void await_resume()
    {
        return;
    }
};