#include "flow_tracing.h"
#include "flow_call_site.h"

Json FlowTracing::get_json_data()
{
    Json data;

    for (const FlowCallSite& callsite : FlowCallSiteManager::all_call_sites())
    {
        Json callsite_data = callsite.get_json_data();
        if (callsite_data == nullptr)
        {
            continue; // Skip empty data
        }

        callsite_data.for_each_with_key([&](const std::string& from, Json& from_data)
        {
            from_data.for_each_with_key([&](const std::string& to, Json& metric_data)
            {
                data[from][to].push_back(metric_data);
            });
        });
    }

    return data;
}