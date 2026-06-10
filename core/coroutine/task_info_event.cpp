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

        if (promise->force_destroy == false)
        {
            promise->handle.resume();
        }
    }
    else if (type == TaskType::REMOVE_AWAITER)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Removing awaiter for promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->has_awaiter = false;
    }
    else if (type == TaskType::FORCE_DESTROY)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Force destroying promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->force_destroy = true;
    }

    if ((promise->force_destroy == true || promise->has_awaiter == false) && promise->has_suspend_value == false)
    {
        // spdlog::warn("EventBase - [TaskInfoEvent::check_handle] Destroying promise: {}, event base id: {}", (void*)promise, promise->m_event_base->m_event_base_id);
        promise->handle.destroy();
        promise = nullptr;
    }
}
