#pragma once

#include <c_json/json_type_base.h>
#include <c_json/json_value.h>

class JsonNew
{
    JsonTypeBaseNew* m_value = nullptr;

public:
    JsonNew();

    JsonNew(const JsonNew& copy) noexcept
    {
        m_value = copy.m_value ? copy.m_value->get_copy() : nullptr;
    }

    JsonNew(JsonNew&& copy) noexcept
    {
        // Transfer ownership
        m_value = copy.m_value;
        copy.m_value = nullptr;
    }

    JsonNew& operator=(const JsonNew& copy) noexcept
    {
        if (this != &copy)
        {
            if (m_value)
            {
                m_value->release();
            }

            m_value = copy.m_value->get_copy();
        }
        return *this;
    }

    JsonNew& operator=(JsonNew&& copy) noexcept
    {
        if (this != &copy)
        {
            if (m_value)
            {
                m_value->release();
            }

            m_value = copy.m_value;
            copy.m_value = nullptr; // Transfer ownership
        }
        return *this;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    JsonNew& operator=(T&& value)
    {
        check_create_json_value();
        ((JsonValueNew*)m_value)->operator=(std::forward<T>(value));
        return *this;
    }

    template<class T>
    operator T()
    {
        if (m_value == nullptr || m_value->is_json_value() == false)
        {
            return T(); // Return default value if not a valid JsonValue
        }
        else
        {
            return ((JsonValueNew*)m_value)->operator T();
        }
    }

    JsonNew& operator[](const char* key);
    JsonNew& operator[](size_t index);

    template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    JsonNew& operator[](T index)
    {
        return (*this)[static_cast<size_t>(index)];
    }

    // Null check
    bool operator==(std::nullptr_t t) const
    {
        return is_null();
    }
    bool is_null() const;

    ~JsonNew()
    {
        if (m_value != nullptr)
        {
            m_value->release();
            m_value = nullptr;
        }
    }

    std::string get_string_value() const
    {
        return m_value ? m_value->get_string_value() : "null";
    }

private:
    void check_create_json_value();
    void check_create_json_object();
};

template <>
struct fmt::formatter<JsonNew> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const JsonNew& json, FormatContext& ctx)
    {
        return fmt::formatter<std::string>::format(json.get_string_value(), ctx);
    }
};