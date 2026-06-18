#pragma once

#include <utils/utils.h>
#include <coroutine/event_base_manager.h>

struct TraceTransfer
{
    EventBaseID from;
    EventBaseID to;

    uint64_t enqueue_ns;
    uint64_t execute_ns;

    void record_enqueue(EventBaseID to_id)
    {
        from = static_cast<EventBaseID>(CURRENT_EVENT_BASE);
        to = to_id;

        enqueue_ns = Utils::get_time_now_in_utc_nanoseconds();
    }

    void record_execute()
    {
        execute_ns = Utils::get_time_now_in_utc_nanoseconds() - enqueue_ns;
    }
};