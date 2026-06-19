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

    template <FixedString File, FixedString Function, size_t Line>
    void record_enqueue(EventBaseID to_id)
    {
        from = CURRENT_EVENT_BASE;
        to = to_id;

        enqueue_ns = Utils::get_time_now_in_utc_nanoseconds();
    }

    template <FixedString File, FixedString Function, size_t Line>
    void record_execute()
    {
        delay_ns = Utils::get_time_now_in_utc_nanoseconds() - enqueue_ns;
        update_metric<File, Function, Line>();
        update_max<File, Function, Line>();
    }

    template <FixedString File, FixedString Function, size_t Line>
    void update_metric()
    {
        auto& metric = FlowTracing::metric<File, Function, Line>(from, to);
        metric.count.fetch_add(1, std::memory_order_relaxed);
        metric.total_delay_ns.fetch_add(delay_ns, std::memory_order_relaxed);
    }

    template <FixedString File, FixedString Function, size_t Line>
    void update_max()
    {
        auto& metric = FlowTracing::metric<File, Function, Line>(from, to);
        auto old = metric.max_delay_ns.load(std::memory_order_relaxed);

        while (delay_ns > old && !metric.max_delay_ns.compare_exchange_weak(old, delay_ns, std::memory_order_relaxed))
        {}
    }
};