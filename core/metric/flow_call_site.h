
#pragma once

#include <string_view>
#include <vector>
#include <iostream>

#include <utils/fixed_string.h>
#include <json/json.h>

#include "trace_transfer.h"

struct FlowCallSite
{
    std::string_view file;
    std::string_view function;
    size_t line;

    Json (*get_json_data)();
};

struct FlowCallSiteManager
{
    static std::vector<FlowCallSite>& all_call_sites()
    {
        static std::vector<FlowCallSite> call_sites;
        return call_sites;
    }

    static void add_call_site(std::string_view file, std::string_view function, size_t line, Json (*get_json_data)())
    {
        all_call_sites().push_back({file, function, line, get_json_data});
    }
};

template <FixedString File, FixedString Function, size_t Line>
struct FlowCallSiteRegister
{
    FlowCallSiteRegister()
    {
        FlowCallSiteManager::add_call_site(File, Function, Line, &FlowTracing::get_json_data_by_call_size<File, Function, Line>);
    }
};

template <FixedString File, FixedString Function, size_t Line>
struct FlowCallSiteKey
{
    static inline FlowCallSiteRegister<File, Function, Line> register_instance;
};
