#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

struct Symbol
{
    const std::string* value;

    Symbol(const std::string& data) : value{get_value(data)}
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

    static std::string* get_value(const std::string& data)
    {
        static std::unordered_map<std::string, std::string> value_list;
        static std::mutex mutex_symbol;

        if (value_list.find(data) == value_list.end())
        {
            std::unique_lock lock(mutex_symbol);
            value_list.insert(std::make_pair(data, data));
        }

        return &value_list[data];
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
};