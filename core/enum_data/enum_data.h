#pragma once

#include <unordered_map>
#include <string>

template<class T>
struct EnumData
{
    static std::unordered_map<T, std::string>& get_enum_to_string_map()
    {
        static std::unordered_map<T, std::string> enum_to_string_map;
        return enum_to_string_map;
    }

    static void add_enum_value(T enum_value, const std::string& string_value)
    {
        get_enum_to_string_map().emplace(enum_value, string_value);
    }

    static std::string to_string(T enum_value)
    {
        auto& map = get_enum_to_string_map();
        auto it = map.find(enum_value);
        if (it != map.end())
        {
            return it->second;
        }
        
        throw std::runtime_error("Cannot find string representation for enum value: " + std::to_string(static_cast<int>(enum_value)));
    }

    static T from_string(const std::string& string_value)
    {
        auto& map = get_enum_to_string_map();
        for (const auto& pair : map)
        {
            if (pair.second == string_value)
            {
                return pair.first;
            }
        }

        throw std::runtime_error("Cannot find enum value for string: " + string_value);
    }
};
