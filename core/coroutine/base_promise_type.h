#pragma once

#include "event_base.h"

struct BasePromiseType
{
    BasePromiseType* m_suspending_promise = nullptr;
    std::coroutine_handle<> handle = nullptr;
    EventBase* m_event_base = nullptr;
    bool has_suspend_value = false;
    bool has_awaiter = true;
    bool force_destroy = false;
    bool is_task_release = false;
    void* task_ptr = nullptr;
};