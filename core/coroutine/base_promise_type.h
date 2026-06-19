#pragma once

#include <metric/trace_transfer.h>
#include "event_base.h"

struct BasePromiseType
{
    BasePromiseType* m_suspending_promise = nullptr;
    std::coroutine_handle<> handle = nullptr;
    EventBase* m_event_base = nullptr;
    bool has_suspend_value = false;
    bool has_awaiter = true;

    // For metric tracing
    std::unique_ptr<TraceTransferParent> trace = nullptr;

private:
    std::atomic<bool> force_destroy{false};

public:
    inline void set_force_destroy()
    {
        force_destroy.store(true, std::memory_order_release);
    }

    inline bool get_force_destroy() const
    {
        return force_destroy.load(std::memory_order_acquire);
    }
};