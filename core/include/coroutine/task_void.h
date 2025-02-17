#ifndef TASK_VOID_H
#define TASK_VOID_H

#include <future>
#include "base_promise_type.h"

struct TaskVoid
{
    struct promise_type : public BasePromiseType
    {
        // Methods of a standard promise
        TaskVoid get_return_object()
        {
            return TaskVoid{std::coroutine_handle<promise_type>::from_promise(*this)};
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
        void return_void()
        {
            promise_value.set_value();
        }
        void unhandled_exception() { std::terminate(); }

        // Promise value
        std::promise<void> promise_value;
    };

    std::coroutine_handle<promise_type> handle;
    TaskVoid(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~TaskVoid()
    {
        auto base_promise_type = get_base_promise_type();
        uint64_t task_id = base_promise_type->task_id;
        base_promise_type->m_event_base->remove_from_event_base(task_id);

        if (handle && handle.done())
        {
            handle.destroy();
        }
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

    std::future<void> start_running_on(EventBase* event_base)
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

    void await_resume()
    {
        return;
    }
};

#endif // TASK_VOID_H