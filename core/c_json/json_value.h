#pragma once

#include <variant>
#include <string>

#include <c_json/json_type_base.h>
#include <cache/cache_pool.h>
#include <cache/share_string.h>

class JsonValueNew;
using JsonValuePool = CachePool<JsonValueNew, 10000>;

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

    virtual bool is_json_value() override
    {
        return true;
    }

    virtual const std::string get_string_value() const override
    {
        return {};
    }

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