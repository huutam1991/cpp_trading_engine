#pragma once

#include <variant>
#include <string>

#include <c_json/json_type_base.h>
#include <cache/cache_pool.h>
#include <cache/share_string.h>

class JsonValue;
using JsonValuePool = CachePool<JsonValue, 10000>;

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
    JsonValue& operator=(const JsonValue& other)
    {
        if (this != &other)
        {
            m_value = other.m_value;
        }
        return *this;
    }

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

    virtual bool is_json_value() override
    {
        return true;
    }

    virtual const std::string get_string_value() const override
    {
        return {};
    }

    virtual JsonTypeBase* get_copy(const JsonTypeBase* other) override
    {
        JsonValue* json_value = JsonValuePool::acquire();
        *json_value = *this;
        return json_value;
    }
};