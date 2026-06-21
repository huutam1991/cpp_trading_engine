#pragma once

#include <source_location>

#include <utils/utils.h>
#include <coroutine/event_base_manager.h>

#include "flow_tracing.h"
#include "flow_call_site.h"

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
    }

    void record_execute()
    {
        if (update_metric_func == nullptr && update_max_func == nullptr)
        {
            return;
        }

        delay_ns = Utils::get_time_now_in_utc_nanoseconds() - enqueue_ns;

        if (update_metric_func != nullptr)
        {
            (this->*update_metric_func)();
        }
    }

    template <FixedString File, FixedString Function, size_t Line>
    void update_metric()
    {
        auto& metric = FlowTracing::metric<File, Function, Line>(from, to);
        metric.record(delay_ns);
    }
};