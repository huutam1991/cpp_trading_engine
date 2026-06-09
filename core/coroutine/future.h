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
        Future<T>* m_future = nullptr;

    public:
        FutureValue(Future<T>* future) : m_future(future) {}
        FutureValue() = default;

        // Only allow move constructor and move assignment
        FutureValue(FutureValue&& copy) : m_future(copy.m_future)
        {
            copy.m_future = nullptr;
        }

        FutureValue& operator=(FutureValue&& copy)
        {
            if (this != &copy)
            {
                m_future = copy.m_future;
                copy.m_future = nullptr;
            }

            return *this;
        }

        // Delete copy constructor and copy assignment operator
        FutureValue(const FutureValue& copy) = delete;
        FutureValue& operator=(const FutureValue&) = delete;

        FutureValue& operator=(std::nullptr_t null)
        {
            m_future = nullptr;
            return *this;
        }

        bool operator==(std::nullptr_t) const
        {
            return m_future == nullptr;
        }

        void set_value(T& value)
        {
            if (m_future != nullptr)
            {
                m_future->set_value(value);
            }

            // Clear the future pointer after setting the value
            m_future = nullptr;
        }

        void set_value(T&& value)
        {
            if (m_future != nullptr)
            {
                m_future->set_value(std::move(value));
            }

            // Clear the future pointer after setting the value
            m_future = nullptr;
        }
    };

private:
    T m_value_object;
    BasePromiseType* m_suspending_promise = nullptr;
    std::function<void(FutureValue)> m_execute_func;

public:
    // Constructor, need to have an execute function
    template <class F, std::enable_if_t<std::is_invocable_v<F, FutureValue>, int> = 0>
    Future(F&& execute_func) : m_execute_func(std::forward<F>(execute_func))
    {
    }

    // Delete other constructors
    template <class U,
        std::enable_if_t<
            !std::is_invocable_v<U, FutureValue> &&
            !std::is_same_v<std::decay_t<U>, FutureValue>,
            int> = 0>
    Future(U& value) = delete;

    template <class U,
        std::enable_if_t<
            !std::is_invocable_v<U, FutureValue> &&
            !std::is_same_v<std::decay_t<U>, FutureValue>,
            int> = 0>
    Future(U&& value) = delete;

    void set_value(T& value)
    {
        m_value_object = value;
        m_suspending_promise->m_event_base->add_set_suspend_value_event(m_suspending_promise);
    }

    void set_value(T&& value)
    {
        m_value_object = std::move(value);
        m_suspending_promise->m_event_base->add_set_suspend_value_event(m_suspending_promise);
    }

    bool await_ready()
    {
        return false;
    }

    template<class promise_type>
    void await_suspend(std::coroutine_handle<promise_type> suspend_handle)
    {
        // Tricky here, cast promise_type to a pointer of BasePromiseType (suppose all of promise_type is child class of BasePromiseType class)
        promise_type& promise = suspend_handle.promise();
        m_suspending_promise = &promise;
        m_suspending_promise->has_suspend_value = true;

        if (m_execute_func)
        {
            m_execute_func(FutureValue(this));
        }
    }

    T await_resume()
    {
        return std::move(m_value_object);
    }
};
