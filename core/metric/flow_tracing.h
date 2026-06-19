#pragma once

#include <json/json.h>
#include <enum_reflect/enum_reflect.h>
#include <coroutine/event_base_manager.h>

#include "flow_metric.h"

class FlowTracing
{
private:
    // struct FlowCallsiteKey
    // {
    //     size_t from = 0;
    //     size_t to = 0;

    //     const char* file = "";
    //     const char* function = "";
    //     uint32_t line = 0;

    //     bool operator==(const FlowCallsiteKey& other) const
    //     {
    //         return from == other.from &&
    //                to == other.to &&
    //                line == other.line &&
    //                std::strcmp(file, other.file) == 0 &&
    //                std::strcmp(function, other.function) == 0;
    //     }
    // };

    // struct FlowCallsiteKeyHash
    // {
    //     size_t operator()(const FlowCallsiteKey& key) const
    //     {
    //         size_t h = 1469598103934665603ULL;

    //         auto mix_u64 = [&](uint64_t v)
    //         {
    //             h ^= v;
    //             h *= 1099511628211ULL;
    //         };

    //         auto mix_str = [&](const char* s)
    //         {
    //             if (s == nullptr)
    //             {
    //                 return;
    //             }

    //             while (*s)
    //             {
    //                 h ^= static_cast<unsigned char>(*s++);
    //                 h *= 1099511628211ULL;
    //             }
    //         };

    //         mix_u64(key.from);
    //         mix_u64(key.to);
    //         mix_u64(key.line);
    //         mix_str(key.file);
    //         mix_str(key.function);

    //         return h;
    //     }
    // };

    // using MetricMap = std::unordered_map<FlowCallsiteKey, std::unique_ptr<FlowMetric>, FlowCallsiteKeyHash>;

public:

    static constexpr size_t MAX_EVENT_BASES = static_cast<size_t>(EventBaseID::TOTAL);

    template <FixedString File, FixedString Function, size_t Line>
    static FlowMetric<File, Function, Line>& metric(EventBaseID from, EventBaseID to)
    {
        static FlowMetric<File, Function, Line> metrics[MAX_EVENT_BASES][MAX_EVENT_BASES];
        return metrics[(size_t)from][(size_t)to];
    }

    template <FixedString File, FixedString Function, size_t Line>
    static Json get_json_data()
    {
        Json data;
        for (size_t from = 0; from < MAX_EVENT_BASES; ++from)
        {
            for (size_t to = 0; to < MAX_EVENT_BASES; ++to)
            {
                const FlowMetric<File, Function, Line>& metric_data = metric<File, Function, Line>(static_cast<EventBaseID>(from), static_cast<EventBaseID>(to));
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