#pragma once

#include <variant>
#include <string>

#include <c_json/json_type_base.h>
#include <cache/share_string.h>

class JsonValue : public JsonTypeBase
{
    std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        size_t,
        double,
        ShareString,
        std::string_view,
        const char*
    > m_value;

public:
    template<class T>
    operator T()
    {
        // If conversion value has the same type of current value, return it
        if (std::holds_alternative<T>(m_value))
        {
            return std::get<T>(m_value);
        }
        // otherwise, return a default value of T
        else
        {
            return T();
        }
    }

    template<class T>
    JsonValue& operator=(T value)
    {
        m_value = value;
        return *this;
    }
};