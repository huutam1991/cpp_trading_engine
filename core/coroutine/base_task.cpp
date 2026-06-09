#include "base_task.h"

bool BaseTask::operator==(std::nullptr_t null) const
{
    return m_promise == nullptr;
}

void BaseTask::destroy(bool complete)
{
    // This is just a BaseTask object with nullptr handle, not a really BaseTask that is created by C++
    if (m_promise == nullptr)
    {
        return;
    }

    // Hasn't register on EventBase, just destroy the coroutine frame and return
    if (m_promise->m_event_base == nullptr)
    {
        m_promise->handle.destroy();
        m_promise = nullptr;
        return;
    }

    // Already register, mark this task is already release, then it will be destroy later when it's done
    m_promise->is_task_release = true;
    m_promise = nullptr;
}

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

void BaseTask::register_on(EventBase* event_base)
{
    m_promise->register_on(event_base, m_promise->handle);

    // m_promise->m_event_base = event_base;
    // event_base->add_run_task_event(m_promise);
}

bool BaseTask::await_ready()
{
    return m_promise->handle.done();
}