#pragma once

#include "event_base.h"

struct BasePromiseType
{
    bool m_is_waiting = false;
    BasePromiseType* m_suspending_promise = nullptr;
    std::coroutine_handle<> handle = nullptr;
    EventBase* m_event_base = nullptr;
    bool has_suspend_value = false;
    bool is_awaiter_release = false;
    bool is_task_release = false;
    void* task_ptr = nullptr;

    void register_on(EventBase* event_base, std::coroutine_handle<> handle)
    {
        m_event_base = event_base;
        this->handle = handle;
        task_ptr = event_base->create_task_info(handle, this);
        set_waiting(false); // Need to run this task at the beginning
    }

    void set_waiting(bool value)
    {
        m_is_waiting = value;

        if (m_is_waiting == false)
        {
            m_event_base->set_ready_task(task_ptr);
        }
    }

    bool is_waiting()
    {
        return m_is_waiting;
    }
};