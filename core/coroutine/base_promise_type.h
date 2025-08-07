#pragma once

#include "event_base.h"

struct BasePromiseType
{
    bool m_is_waiting = false;
    BasePromiseType* m_suspending_promise = nullptr;
    EventBase* m_event_base = nullptr;
    bool is_task_release = false;
    void* task_ptr = nullptr;

    void register_on(EventBase* event_base, std::coroutine_handle<> handle)
    {
        m_event_base = event_base;
        task_ptr = event_base->add_to_event_base(handle, this);
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