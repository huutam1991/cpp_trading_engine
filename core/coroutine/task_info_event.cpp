#include "task_info_event.h"
#include "base_promise_type.h"

void TaskInfoEvent::check_handle()
{
    if (promise == nullptr)
    {
        spdlog::error("EventBase - [TaskInfoEvent::check_handle] Promise is nullptr");
        return;
    }

    if (type == TaskType::RUN)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Running task with promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->handle.resume();
    }
    else if (type == TaskType::SET_SUSPEND_VALUE)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Setting suspend value for promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->has_suspend_value = false;

        if (promise->has_awaiter == true)
        {
            promise->handle.resume();
        }
    }
    else if (type == TaskType::REMOVE_AWAITER)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Removing awaiter for promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->has_awaiter = false;
    }

    if (promise->has_awaiter == false && promise->has_suspend_value == false)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Destroying promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->handle.destroy();
        promise = nullptr;
    }
}

int TaskInfoEvent::generate_fd()
{
    fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return fd;
}

int TaskInfoEvent::activate()
{
    // Nothing to do for TaskInfoEvent
    return 0;
}

int TaskInfoEvent::handle_read()
{
    check_handle();

    // Always return -1 to indicate this task is done
    return -1;
}

int TaskInfoEvent::handle_write()
{
    // Nothing to do for write event
    return 0;
}

void TaskInfoEvent::release()
{
    if (this != nullptr)
    {
        delete this;
    }
}
