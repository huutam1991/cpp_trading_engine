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
    BaseTask(promise_type* promise) : m_promise((BasePromiseType*)promise) {}
    BaseTask() {};

    // Only allow move constructor and move assignment
    BaseTask(BaseTask&& copy) : m_promise{copy.m_promise} { copy.m_promise = nullptr; }
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

    // Delete copy constructor and copy assignment
    BaseTask(const BaseTask& copy) = delete;
    BaseTask& operator=(const BaseTask& copy) = delete;

    ~BaseTask()
    {
        // Light destroy, lol
        destroy(false);
    }

    bool operator==(std::nullptr_t null) const;
    void destroy(bool complete = true);
    void check_release();
    void save_suspending_promise(BasePromiseType* suspend_base_pt);
    void register_on(EventBase* event_base);
    bool await_ready();

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