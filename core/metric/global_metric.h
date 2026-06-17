#pragma once

#include "flow_metric.h"
#include <coroutine/event_base_manager.h>

class FlowTracing
{
public:

    static constexpr size_t MAX_EVENT_BASES = static_cast<size_t>(EventBaseID::TOTAL);

    static FlowMetric& metric(EventBaseID from, EventBaseID to)
    {
        static FlowMetric metrics[MAX_EVENT_BASES][MAX_EVENT_BASES];
        return metrics[(size_t)from][(size_t)to];
    }
};