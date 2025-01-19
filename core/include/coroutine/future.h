#ifndef FUTURE_H
#define FUTURE_H

#include <functional>
#include "base_promise_type.h"

// Future is not a coroutine, it's just a awaitable
template<class T>
struct Future
{
    class FutureValue
    {
    private:
        std::shared_ptr<T> m_value;
        BasePromiseType* m_suspending_promise = nullptr;

    public:
        FutureValue() : m_value{std::make_shared<T>()}
        {}

        void set_suspending_promise(BasePromiseType* suspending_promise)
        {
            m_suspending_promise = suspending_promise;
        }

        void set_value(T& value)
        {
            *m_value = value;

            // Mark future as ready
            future_set_ready();
        }

        void set_value(T&& value)
        {
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
        }

    };

    FutureValue m_value;
    std::function<void(FutureValue)> m_execute_func;

    // Constructor, need to have an execute function
    Future(std::function<void(FutureValue)> execute_func) : m_execute_func(execute_func)
    {
    }

    bool await_ready()
    {
        return false; // Always false as it will be ready in future
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

#endif // FUTURE_H