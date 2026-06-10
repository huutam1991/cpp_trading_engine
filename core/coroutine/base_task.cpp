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
            spdlog::warn("BaseTask - [check_release] Removing awaiter for task with promise: {}, event base id: {}", (void*)m_promise, m_promise->m_event_base->m_event_base_id);
            m_promise->m_event_base->add_remove_awaiter_event(m_promise);
        }

        m_promise = nullptr;
    }
}

void BaseTask::destroy()
{
    spdlog::warn("BaseTask - [destroy] Force destroying task with promise: {}, event base id: {}", (void*)m_promise, m_promise != nullptr && m_promise->m_event_base != nullptr ? m_promise->m_event_base->m_event_base_id : 0);
    if (m_promise != nullptr && m_promise->m_event_base != nullptr)
    {
        spdlog::warn("BaseTask - [destroy] Force destroying task with promise: {}, event base id: {}", (void*)m_promise, m_promise->m_event_base->m_event_base_id);
        m_promise->m_event_base->add_force_destroy_event(m_promise);
        m_promise = nullptr;
    }
}

void BaseTask::register_on(EventBase* event_base)
{
    m_promise->m_event_base = event_base;
    event_base->add_run_task_event(m_promise);
}

bool BaseTask::await_ready()
{
    return m_promise->handle.done();
}