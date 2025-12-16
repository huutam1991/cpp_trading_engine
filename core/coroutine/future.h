#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "base_promise_type.h"

// Future is not a coroutine, it's just a awaitable
template<class T>
struct Future
{
    class FutureValue
    {
    private:
        T m_value;
        std::atomic<bool> m_is_set = false;
        std::atomic<BasePromiseType*> m_suspending_promise = nullptr;

    public:
        FutureValue() = default;

        // Only allow move constructor and delete copy constructor
        FutureValue(const FutureValue& copy) = delete;
        FutureValue(FutureValue&& copy) = delete;

        // Only allow move assignment and delete copy assignment
        FutureValue& operator=(const FutureValue&) = delete;
        FutureValue& operator=(FutureValue&&) = delete;

        void set_suspending_promise(BasePromiseType* suspending_promise)
        {
            m_suspending_promise.store(suspending_promise, std::memory_order_release);
        }

        bool is_value_set()
        {
            return m_is_set.load(std::memory_order_acquire);
        }

        void set_value(T& value)
        {
            // Check if this future is already set
            if (m_is_set.load(std::memory_order_acquire) == true) return;

            m_value = value;

            // Mark future as ready
            future_set_ready();
        }

        void set_value(T&& value)
        {
            // Check if this future is already set
            if (m_is_set.load(std::memory_order_acquire) == true) return;

            m_value = std::move(value);

            // Mark future as ready
            future_set_ready();
        }

        T get_value()
        {
            return std::move(m_value);
        }

    private:
        void future_set_ready()
        {
            m_is_set.store(true, std::memory_order_release);

            // Mark suspending promise as ready
            BasePromiseType* suspending_promise = m_suspending_promise.load(std::memory_order_acquire);
            if (suspending_promise != nullptr)
            {
                suspending_promise->set_waiting(false);
            }
        }

    };

    FutureValue m_value;
    std::function<void(FutureValue*)> m_execute_func;

    // Constructor, need to have an execute function
    template <class F, std::enable_if_t<std::is_invocable_v<F, FutureValue*>, int> = 0>
    Future(F&& execute_func) : m_execute_func(std::forward<F>(execute_func))
    {
    }

    // Delete other constructors
    template <class U,
        std::enable_if_t<
            !std::is_invocable_v<U, FutureValue*> &&
            !std::is_same_v<std::decay_t<U>, FutureValue>,
            int> = 0>
    Future(U& value) = delete;

    template <class U,
        std::enable_if_t<
            !std::is_invocable_v<U, FutureValue*> &&
            !std::is_same_v<std::decay_t<U>, FutureValue>,
            int> = 0>
    Future(U&& value) = delete;

    bool await_ready()
    {
        return false;
    }

    template<class promise_type>
    void await_suspend(std::coroutine_handle<promise_type> suspend_handle)
    {
        // Tricky here, cast promise_type to a pointer of BasePromiseType (suppose all of promise_type is child class of BasePromiseType class)
        promise_type& promise = suspend_handle.promise();
        BasePromiseType *suspend_base_pt = &promise;
        suspend_base_pt->set_waiting(true);

        m_value.set_suspending_promise(suspend_base_pt);

        if (m_execute_func)
        {
            m_execute_func(&m_value);
        }
    }

    T await_resume()
    {
        return m_value.get_value();
    }
};
