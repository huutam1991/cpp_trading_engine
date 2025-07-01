#pragma once

#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <unordered_map>
#include <utils/spin_lock.h>

struct Symbol
{
    const std::string* value;

    explicit Symbol(const std::string& data) : value{get_value(data)} 
    {}
    Symbol() : value{get_value("BTC")} // Just a default symbol
    {}

    Symbol(const Symbol& copy) : value{copy.value} {}
    Symbol(Symbol&& copy) : value{copy.value} {}
    Symbol& operator=(const Symbol& copy)
    {
        value = copy.value;
        return *this;
    }
    Symbol& operator=(Symbol&& copy)
    {
        value = copy.value;
        return *this;
    }

    static const std::string* get_value(const std::string& data)
    {
        static std::unordered_map<std::string, std::string> value_list;
        static SpinLock spin_lock_symbol;

        SpinLockGuard lock(spin_lock_symbol);

        auto it = value_list.find(data);
        if (it != value_list.end()) 
        {
            return &it->second;
        }

        auto [insert, _] = value_list.emplace(data, data);
        return &insert->second;
    }

    operator std::string() const
    {
        return *value;
    }

    const std::string& to_string() const
    {
        return *value;
    }

    Symbol& operator=(const std::string& data)
    {
        value = get_value(data);
        return *this;
    }

    bool operator==(const Symbol& symbol) const
    {
        if (value == symbol.value) 
        {
            return true;
        }

        return *value == *symbol.value;
    }

    bool operator==(const std::string& str) const
    {
        return *value == str;
    }
};

template <>
struct fmt::formatter<Symbol> : fmt::formatter<std::string> 
{
    template <typename FormatContext>
    auto format(const Symbol& data, FormatContext& ctx) 
    {
        return fmt::formatter<std::string>::format(data.to_string(), ctx);
    }
};