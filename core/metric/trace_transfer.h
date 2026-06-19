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

    void (TraceTransfer::*update_metric_func)() = nullptr;
    void (TraceTransfer::*update_max_func)() = nullptr;

    template <FixedString File, FixedString Function, size_t Line>
    void record_enqueue(EventBaseID to_id)
    {
        from = CURRENT_EVENT_BASE;
        to = to_id;

        enqueue_ns = Utils::get_time_now_in_utc_nanoseconds();

        update_metric_func = &TraceTransfer::update_metric<File, Function, Line>;
        update_max_func = &TraceTransfer::update_max<File, Function, Line>;
    }

    void record_execute()
    {
        delay_ns = Utils::get_time_now_in_utc_nanoseconds() - enqueue_ns;

        if (update_metric_func != nullptr)
        {
            (this->*update_metric_func)();
        }
        if (update_max_func != nullptr)
        {
            (this->*update_max_func)();
        }
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