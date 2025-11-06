#pragma once

#include <future>
#include "base_promise_type.h"

template<class T>
struct BaseTask
{
    struct promise_type : public BasePromiseType
    {
        // Methods of a standard promise
        BaseTask get_return_object()
        {
            return BaseTask{std::coroutine_handle<promise_type>::from_promise(*this)};
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
        void unhandled_exception() { std::terminate(); }

        // Promise value
        std::promise<T> promise_value;
    };

    std::coroutine_handle<promise_type> handle = nullptr;
    BaseTask(std::coroutine_handle<promise_type> h) : handle(h) {}
    BaseTask(promise_type* promise) : handle(std::coroutine_handle<promise_type>::from_promise(*promise)) {}
    BaseTask() {};
    BaseTask(const BaseTask& copy) = delete;
    BaseTask(BaseTask&& copy) : handle{std::move(copy.handle)} { copy.handle = nullptr; }
    ~BaseTask()
    {
        // Light destroy, lol
        destroy(false);
    }

    bool operator==(std::nullptr_t null) const
    {
        return handle == nullptr;
    }

    BaseTask& operator=(const BaseTask& copy) = delete;

    BaseTask& operator=(BaseTask&& copy)
    {
        if (handle != nullptr)
        {
            destroy();
        }

        handle = std::move(copy.handle);
        copy.handle = nullptr;
        return *this;
    }

    void destroy(bool complete = true)
    {
        // This is just a BaseTask object with nullptr handle, not a really BaseTask that is created by C++
        if (handle == nullptr)
        {
            return;
        }

        if (handle.done() || complete == true)
        {
            auto base_promise_type = get_base_promise_type();
            if (base_promise_type->m_event_base != nullptr)
            {
                void* task_ptr = base_promise_type->task_ptr;
                base_promise_type->m_event_base->remove_from_event_base(task_ptr);
            }

            // handle.destroy(); // Will be destroyed at EventBase
        }
        else
        {
            // Mark this task is already release, then it will be destroy later when it's done
            get_base_promise_type()->is_task_release = true;
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

    inline std::future<T> start_running_on(EventBase* event_base)
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
};