#include "base_task.h"

void BaseTask::check_release()
{
    if (m_promise != nullptr)
    {
        // If this task is not running on any EventBase, we can destroy it immediately
        if (m_promise->m_event_base == nullptr)
        {
            m_promise->handle.destroy();
        }
        // If this task is running on an EventBase, mark it as has no waiter by sending an event to that EventBase
        else
        {
            m_promise->m_event_base->add_remove_awaiter_event(m_promise);
        }

        m_promise = nullptr;
    }
}

void BaseTask::destroy()
{
    if (m_promise != nullptr && m_promise->m_event_base != nullptr)
    {
        m_promise->set_force_destroy();
        // m_promise->m_event_base->add_force_destroy_event(m_promise);
        m_promise = nullptr;
    }
}

bool BaseTask::await_ready()
{
    return m_promise->handle.done();
}