#pragma once

#include <future>
#include "task_void.h"
#include "base_promise_type.h"

template<class T>
struct Task
{
    struct promise_type : public BasePromiseType
    {
        // Methods of a standard promise
        Task get_return_object()
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept
        {
            if (m_suspending_promise != nullptr)
            {
                m_suspending_promise->set_waiting(false);
            }

            return {};
        }
        void return_value(T v)
        {
            value = v;
            promise_value.set_value(v);
        }
        void unhandled_exception() { std::terminate(); }

        // Main value
        T value;

        // Promise value
        std::promise<T> promise_value;
    };

    std::coroutine_handle<promise_type> handle = nullptr;
    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    Task() {};
    Task(const Task& copy) : handle{copy.handle} {}
    ~Task()
    {
        // Light destroy, lol
        destroy(false);
    }

    Task& operator=(const Task& copy)
    {
        handle = copy.handle;
        return *this;
    }

    void destroy(bool complete = true)
    {
        // This is just a Task object with nullptr handle, not a really Task that is created by C++
        if (handle == nullptr)
        {
            return;
        }

        if (handle.done() || complete == true)
        {
            auto base_promise_type = get_base_promise_type();
            if (base_promise_type->m_event_base != nullptr)
            {
                uint64_t task_id = base_promise_type->task_id;
                base_promise_type->m_event_base->remove_from_event_base(task_id);
            }

            handle.destroy();
        }
        else
        {
            // Mark this task is already release, then it will be destroy later when it's done
            get_base_promise_type()->is_task_release = true;
        }
    }

    T value()
    {
        return handle.promise().value;
    }

    // Get BasePromiseType of current coroutine
    BasePromiseType* get_base_promise_type()
    {
        promise_type& promise = handle.promise();
        return &promise;
    }

    void save_suspending_promise(BasePromiseType* suspend_base_pt)
    {
        get_base_promise_type()->m_suspending_promise = suspend_base_pt;
    }

    void register_on(EventBase* event_base)
    {
        auto base_promise_type = get_base_promise_type();
        base_promise_type->register_on(event_base, handle);
    }

    std::future<T> start_running_on(EventBase* event_base)
    {
        register_on(event_base);

        // Return future
        promise_type& promise = handle.promise();
        return promise.promise_value.get_future();
    }

    bool await_ready()
    {
        return handle.done();
    }

    template<class promise_type>
    void await_suspend(std::coroutine_handle<promise_type> suspend_handle)
    {
        // Tricky here, cast promise_type to a pointer of BasePromiseType (suppose all of promise_type is child class of BasePromiseType class)
        promise_type& promise = suspend_handle.promise();
        BasePromiseType *suspend_base_pt = &promise;
        suspend_base_pt->set_waiting(true);

        // Save to suspending_promise
        save_suspending_promise(suspend_base_pt);

        // Running this task on EventBase
        register_on(suspend_base_pt->m_event_base);
    }

    T await_resume()
    {
        return handle.promise().value;
    }
};