#pragma once

#include <variant>
#include <string>

#include <json/json_type_base.h>
#include <cache/cache_pool.h>
#include <cache/share_string.h>

class JsonValue;
using JsonValuePool = CachePool<JsonValue, 1000000>;

class JsonValue : public JsonTypeBase
{
    std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        uint64_t,
        double,
        ShareString
    > m_value;

    bool m_is_string_format = true; // Indicates if value is string, it should has string format
    char buffer_number[50]; // Buffer for number conversion

public:
    JsonValue() = default;
    JsonValue(const JsonValue&) = delete;
    JsonValue(JsonValue&&) = delete;
    JsonValue& operator=(const JsonValue&) = delete;
    JsonValue& operator=(JsonValue&&) = delete;

    virtual ~JsonValue() override = default;

    template<class T>
    operator T() const
    {
        return std::visit([](auto&& arg) -> T
        {
            using U = std::decay_t<decltype(arg)>;
            if constexpr (std::is_convertible_v<U, T>)
            {
                return static_cast<T>(arg);
            }
            else
            {
                return T(); // Return default value if conversion is not possible
            }
        }, m_value);
    }

    template<class T>
    JsonValue& operator=(T&& value)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
        {
            m_value = ShareString(std::forward<T>(value));
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, const char*>)
        {
            m_value = ShareString(std::forward<T>(value));
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>)
        {
            m_value = ShareString(std::string(std::forward<T>(value)));
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, double>)
        {
            m_value = static_cast<double>(std::forward<T>(value));
        }
        else if constexpr (std::is_convertible_v<std::decay_t<T>, int64_t>)
        {
            m_value = static_cast<int64_t>(std::forward<T>(value));
        }
        else
        {
            m_value = std::forward<T>(value);
        }
        return *this;
    }

    void set_is_string_format(bool val)
    {
        m_is_string_format = val;
    }

    bool is_string() const
    {
        return std::holds_alternative<ShareString>(m_value);
    }

    bool is_null() const
    {
        return std::holds_alternative<std::nullptr_t>(m_value);
    }

    // Methosds from JsonTypeBase
    virtual bool is_json_value() override
    {
        return true;
    }

    virtual void write_string_value(JsonStringBuilder& builder) override;

    virtual JsonTypeBase* get_copy() override
    {
        JsonValue* json_value = JsonValuePool::acquire();
        json_value->m_value = m_value;
        json_value->m_is_string_format = m_is_string_format;
        return json_value;
    }

    virtual JsonTypeBase* get_deep_clone() override
    {
        return get_copy(); // For simplicity, deep clone is same as copy in this case
    }

    virtual void release() override
    {
        m_value = nullptr; // Clear the value
        JsonValuePool::release(this);
    }
};