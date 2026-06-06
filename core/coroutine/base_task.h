#pragma once

#include "base_promise_type.h"

struct BaseTask
{
    struct promise_type : public BasePromiseType
    {
        // Methods of a standard promise
        BaseTask get_return_object()
        {
            return BaseTask{nullptr};
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
    };

    // std::coroutine_handle<promise_type> handle = nullptr;
    BasePromiseType* m_promise = nullptr;

    BaseTask(std::nullptr_t) : m_promise(nullptr) {}
    // BaseTask(std::coroutine_handle<promise_type> h) : handle(h) {}
    BaseTask(promise_type* promise) : m_promise((BasePromiseType*)promise) {}
    BaseTask() {};
    BaseTask(const BaseTask& copy) = delete;
    BaseTask(BaseTask&& copy) : m_promise{copy.m_promise} { copy.m_promise = nullptr; }
    ~BaseTask()
    {
        // Light destroy, lol
        destroy(false);
    }

    bool operator==(std::nullptr_t null) const
    {
        return m_promise == nullptr;
    }

    BaseTask& operator=(const BaseTask& copy) = delete;

    BaseTask& operator=(BaseTask&& copy)
    {
        if (m_promise != nullptr)
        {
            destroy();
        }

        m_promise = copy.m_promise;
        copy.m_promise = nullptr;
        return *this;
    }

    void destroy(bool complete = true)
    {
        // This is just a BaseTask object with nullptr handle, not a really BaseTask that is created by C++
        if (m_promise == nullptr)
        {
            return;
        }

        auto* promise = get_base_promise_type();

        // Hasn't register on EventBase, just destroy the coroutine frame and return
        if (promise->m_event_base == nullptr)
        {
            m_promise->handle.destroy();
            m_promise = nullptr;
            return;
        }

        // Already register, mark this task is already release, then it will be destroy later when it's done
        promise->is_task_release = true;
        m_promise = nullptr;
    }

    // Get BasePromiseType of current coroutine
    BasePromiseType* get_base_promise_type()
    {
        return m_promise;
    }

    void save_suspending_promise(BasePromiseType* suspend_base_pt)
    {
        get_base_promise_type()->m_suspending_promise = suspend_base_pt;
    }

    void register_on(EventBase* event_base)
    {
        auto base_promise_type = get_base_promise_type();
        base_promise_type->register_on(event_base, m_promise->handle);
    }

    // inline TaskResult<T> start_running_on(EventBase* event_base)
    // {
    //     register_on(event_base);
    //     return TaskResult<T>{&handle.promise().task_value};
    // }

    bool await_ready()
    {
        return m_promise->handle.done();
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