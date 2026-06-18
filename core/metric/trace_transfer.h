#pragma once

#include <source_location>

#include <utils/utils.h>
#include <coroutine/event_base_manager.h>

#include "flow_tracing.h"

struct TraceTransfer
{
    EventBaseID from;
    EventBaseID to;

    uint64_t enqueue_ns;
    uint64_t delay_ns;

    void record_enqueue(EventBaseID to_id, const std::source_location& loc)
    {
        from = static_cast<EventBaseID>(CURRENT_EVENT_BASE);
        to = to_id;

        enqueue_ns = Utils::get_time_now_in_utc_nanoseconds();
    }

    void record_execute()
    {
        delay_ns = Utils::get_time_now_in_utc_nanoseconds() - enqueue_ns;
        update_metric();
        update_max();
    }

    void update_metric()
    {
        auto& metric = FlowTracing::metric(from, to);
        metric.count.fetch_add(1, std::memory_order_relaxed);
        metric.total_delay_ns.fetch_add(delay_ns, std::memory_order_relaxed);
    }

    void update_max()
    {
        auto& metric = FlowTracing::metric(from, to);
        auto old = metric.max_delay_ns.load(std::memory_order_relaxed);

        while (delay_ns > old && !metric.max_delay_ns.compare_exchange_weak(old, delay_ns, std::memory_order_relaxed))
        {}
    }
};