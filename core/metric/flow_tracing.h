#pragma once

#include <json/json.h>
#include <enum_reflect/enum_reflect.h>
#include <coroutine/event_base_manager.h>

#include "flow_metric.h"

class FlowTracing
{
public:

    static constexpr size_t MAX_EVENT_BASES = static_cast<size_t>(EventBaseID::TOTAL);

    static FlowMetric& metric(EventBaseID from, EventBaseID to)
    {
        static FlowMetric metrics[MAX_EVENT_BASES][MAX_EVENT_BASES];
        return metrics[(size_t)from][(size_t)to];
    }

    static Json get_json_data()
    {
        Json data;
        for (size_t from = 0; from < MAX_EVENT_BASES; ++from)
        {
            for (size_t to = 0; to < MAX_EVENT_BASES; ++to)
            {
                const FlowMetric& metric_data = metric(static_cast<EventBaseID>(from), static_cast<EventBaseID>(to));
                if (metric_data.count.load(std::memory_order_relaxed) > 0)
                {
                    Json metric_json;
                    metric_json["count"] = metric_data.count.load(std::memory_order_relaxed);
                    metric_json["bytes"] = metric_data.bytes.load(std::memory_order_relaxed);
                    metric_json["total_delay_ns"] = metric_data.total_delay_ns.load(std::memory_order_relaxed);
                    metric_json["max_delay_ns"] = metric_data.max_delay_ns.load(std::memory_order_relaxed);

                    std::string_view from_str = enum_reflect::enum_name(static_cast<EventBaseID>(from));
                    std::string_view to_str = enum_reflect::enum_name(static_cast<EventBaseID>(to));

                    data[from_str][to_str] = std::move(metric_json);
                }
            }
        }

        return data;
    }

};