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
        std::shared_ptr<T> m_value;
        std::shared_ptr<bool> m_is_set;
        std::shared_ptr<std::mutex> m_mutex_future;
        BasePromiseType* m_suspending_promise = nullptr;

    public:
        FutureValue() : m_value{std::make_shared<T>()}, m_is_set{std::make_shared<bool>(false)}, m_mutex_future{std::make_shared<std::mutex>()}
        {}

        FutureValue(const FutureValue& copy) : m_value{copy.m_value}, m_is_set{copy.m_is_set}, m_mutex_future{copy.m_mutex_future}, m_suspending_promise{copy.m_suspending_promise}
        {}

        FutureValue& operator=(const FutureValue& copy)
        {
            m_value = copy.m_value;
            m_is_set = copy.m_is_set;
            m_mutex_future = copy.m_mutex_future;
            m_suspending_promise = copy.m_suspending_promise;

            return *this;
        }

        void set_suspending_promise(BasePromiseType* suspending_promise)
        {
            m_suspending_promise = suspending_promise;
        }

        bool is_value_set()
        {
            std::unique_lock lock(*m_mutex_future);
            return *m_is_set;
        }

        void set_value(T& value)
        {
            std::unique_lock lock(*m_mutex_future);

            // Check if this future is already set
            if (*m_is_set == true) return;

            *m_value = value;

            // Mark future as ready
            future_set_ready();
        }

        void set_value(T&& value)
        {
            std::unique_lock lock(*m_mutex_future);

            // Check if this future is already set
            if (*m_is_set == true) return;

            *m_value = std::move(value);

            // Mark future as ready
            future_set_ready();
        }

        T get_value()
        {
            return *m_value;
        }

    private:
        void future_set_ready()
        {
            // Mark suspending promise as ready
            if (m_suspending_promise != nullptr)
            {
                m_suspending_promise->set_waiting(false);
            }

            *m_is_set = true;
        }

    };

    FutureValue m_value;
    std::function<void(FutureValue)> m_execute_func;

    // Constructor, need to have an execute function
    Future(std::function<void(FutureValue)> execute_func) : m_execute_func(execute_func)
    {
    }
    // Or with a value (ready Future)
    Future(T& value)
    {
        m_value.set_value(value);
    }

    bool await_ready()
    {
        return m_value.is_value_set(); // If value is set, this Future is ready
    }

    template<class promise_type>
    void await_suspend(std::coroutine_handle<promise_type> suspend_handle)
    {
        // Tricky here, cast promise_type to a pointer of BasePromiseType (suppose all of promise_type is child class of BasePromiseType class)
        promise_type& promise = suspend_handle.promise();
        BasePromiseType *suspend_base_pt = &promise;
        suspend_base_pt->set_waiting(true);

        m_value.set_suspending_promise(suspend_base_pt);

        if (m_execute_func) {
            m_execute_func(m_value);
        }
    }

    T await_resume()
    {
        return m_value.get_value();
    }
};
