#pragma once

#include <coroutine/event_base_manager.h>

struct TraceTransfer
{
    EventBaseID from;
    EventBaseID to;

    uint64_t enqueue_ns;
    uint64_t execute_ns;

    // void enqueue()
    // {
    //     from = CURRENT_EVENT_BASE;
    // }
};