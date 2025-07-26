#pragma once

#include <variant>
#include <string>

#include <c_json/json_type_base.h>
#include <cache/cache_pool.h>
#include <cache/share_string.h>

class JsonValueNew;
using JsonValuePool = CachePool<JsonValueNew, 100000>;

class JsonValueNew : public JsonTypeBaseNew
{
    std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        uint64_t,
        double,
        ShareString,
        std::string_view,
        const char*
    > m_value;

    bool m_is_string_format = true; // Indicates if value is string, it should has string format
    char buffer_number[50]; // Buffer for number conversion

public:
    JsonValueNew() = default;
    JsonValueNew(const JsonValueNew&) = delete;
    JsonValueNew(JsonValueNew&&) = delete;
    JsonValueNew& operator=(const JsonValueNew&) = delete;
    JsonValueNew& operator=(JsonValueNew&&) = delete;

    virtual ~JsonValueNew() override = default;

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
    JsonValueNew& operator=(T&& value)
    {
        m_value = std::forward<T>(value);
        return *this;
    }

    bool is_null() const
    {
        return std::holds_alternative<std::nullptr_t>(m_value);
    }

    // Methosds from JsonTypeBaseNew
    virtual bool is_json_value() override
    {
        return true;
    }

    virtual void write_string_value(JsonStringBuilder& builder) override;

    virtual JsonTypeBaseNew* get_copy() override
    {
        JsonValueNew* json_value = JsonValuePool::acquire();
        json_value->m_value = m_value;
        return json_value;
    }

    virtual void release() override
    {
        JsonValuePool::release(this);
    }
};