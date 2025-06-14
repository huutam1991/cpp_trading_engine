#pragma once

#include <string>
#include <unordered_map>

struct Symbol
{
    std::string* value;

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

        if (value_list.find(data) == value_list.end())
        {
            value_list.insert(std::make_pair(data, data));
        }

        return &value_list[data];
    }

    operator std::string()
    {
        return *value;
    }

    Symbol& operator=(const std::string& data)
    {
        value = get_value(data);
        return *this;
    }

    bool operator==(const std::string& data)
    {
        return *value == data;
    }

    bool operator==(const Symbol& symbol)
    {
        if (value == symbol.value) 
        {
            return true;
        }

        return *value == *symbol.value;
    }
};