#pragma once

#include "base_promise_type.h"

#include <mongo_db/mongo_db.h>

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
        std::suspend_always final_suspend() noexcept { return {};}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    // std::coroutine_handle<promise_type> handle = nullptr;
    BasePromiseType* m_promise = nullptr;

    BaseTask(std::nullptr_t) : m_promise(nullptr) {}
    BaseTask(promise_type* promise) : m_promise((BasePromiseType*)promise) {}
    BaseTask() {};

    // Only allow move constructor and move assignment
    BaseTask(BaseTask&& copy) : m_promise{copy.m_promise} { copy.m_promise = nullptr; }
    BaseTask& operator=(BaseTask&& copy)
    {
        if (this == &copy)
        {
            return *this;
        }

        if (m_promise != nullptr)
        {
            check_release();
        }

        m_promise = copy.m_promise;
        copy.m_promise = nullptr;
        return *this;
    }

    // Delete copy constructor and copy assignment
    BaseTask(const BaseTask& copy) = delete;
    BaseTask& operator=(const BaseTask& copy) = delete;

    BaseTask& operator=(std::nullptr_t null)
    {
        check_release();
        return *this;
    }

    bool operator==(std::nullptr_t null) const
    {
        return m_promise == nullptr;
    }

    ~BaseTask()
    {
        check_release();
    }

    void check_release();
    void destroy();
    bool await_ready();

    template <FixedString File, FixedString Function, std::size_t Line>
    void register_on(EventBase* event_base)
    {
        m_promise->m_event_base = event_base;
        m_promise->trace.record_enqueue<File, Function, Line>(event_base->m_event_base_id);

        event_base->add_run_task_event(m_promise);
    }

    void run_on(EventBase* event_base)
    {
        m_promise->m_event_base = event_base;
        event_base->add_run_task_event(m_promise);
    }

    template<class promise_type>
    void await_suspend(std::coroutine_handle<promise_type> suspend_handle)
    {
        // Tricky here, cast promise_type to a pointer of BasePromiseType (suppose all of promise_type is child class of BasePromiseType class)
        promise_type& promise = suspend_handle.promise();
        BasePromiseType *suspend_base_pt = &promise;
        suspend_base_pt->has_suspend_value = true;

        // Save to suspending_promise
        m_promise->m_suspending_promise = suspend_base_pt;

        // Running this task on EventBase
        run_on(suspend_base_pt->m_event_base);
    }
};