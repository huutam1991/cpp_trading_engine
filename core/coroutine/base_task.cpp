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

void BaseTask::check_release()
{
    if (m_promise != nullptr)
    {
        // If this task is not running on any EventBase, we can destroy it immediately
        if (m_promise->m_event_base == nullptr)
        {
            m_promise->handle.destroy();
        }
        // If this task is running on an EventBase, mark it as has no waiter
        else
        {
            m_promise->has_awaiter = false;
        }

        m_promise = nullptr;
    }
}

// Get BasePromiseType of current coroutine
BasePromiseType* BaseTask::get_base_promise_type()
{
    return m_promise;
}

void BaseTask::save_suspending_promise(BasePromiseType* suspend_base_pt)
{
    get_base_promise_type()->m_suspending_promise = suspend_base_pt;
}

void BaseTask::register_on(EventBase* event_base)
{
    auto base_promise_type = get_base_promise_type();
    base_promise_type->register_on(event_base, m_promise->handle);
}

bool BaseTask::await_ready()
{
    return m_promise->handle.done();
}