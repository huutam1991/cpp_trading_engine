
#pragma once

#include <string_view>
#include <vector>
#include <iostream>

#include <utils/fixed_string.h>

struct FlowCallSite
{
    std::string_view file;
    std::string_view function;
    size_t line;

    FlowCallSite(std::string_view file, std::string_view function, size_t line)
        : file(file), function(function), line(line)
    {}
};

struct FlowCallSiteManager
{
    static std::vector<FlowCallSite>& all_call_sites()
    {
        static std::vector<FlowCallSite> call_sites;
        return call_sites;
    }

    static void add_call_site(std::string_view file, std::string_view function, size_t line)
    {
        std::cout << "Adding call site: " << file << ":" << function << ":" << line << std::endl;
        all_call_sites().emplace_back(file, function, line);
    }
};

template <FixedString File, FixedString Function, size_t Line>
struct FlowCallSiteRegister
{
    FlowCallSiteRegister()
    {
        FlowCallSiteManager::add_call_site(File, Function, Line);
    }
};

template <FixedString File, FixedString Function, size_t Line>
struct FlowCallSiteKey
{
    static inline FlowCallSiteRegister<File, Function, Line> register_instance;
};