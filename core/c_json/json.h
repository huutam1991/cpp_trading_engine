#pragma once

#include <c_json/json_type_base.h>
#include <c_json/json_value.h>

#define STRING_BUFFER_SIZE 20000 // Reserve space for 20000 characters

class JsonNew
{
    JsonTypeBaseNew* m_value = nullptr;

public:
    JsonNew();
    JsonNew(std::initializer_list<std::pair<std::string, JsonNew>> json_list);

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
    JsonNew& operator[](const std::string& key);
    JsonNew& operator[](size_t index);

    template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    JsonNew& operator[](T index)
    {
        return (*this)[static_cast<size_t>(index)];
    }

    void for_each(std::function<void(JsonNew&)> loop_func);
    void for_each_with_key(std::function<void(const std::string&,JsonNew&)> loop_func);
    void for_each_with_index(std::function<void(size_t,JsonNew&)> loop_func);

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
        char buffer[STRING_BUFFER_SIZE];

        // Write the JSON value to the buffer
        JsonStringBuilder builder(buffer);
        write_string_value(builder);

        return builder.finish();
    }

    void write_string_value(JsonStringBuilder& builder) const
    {
        if (m_value)
        {
            m_value->write_string_value(builder);
        }
        else
        {
            builder.write_raw("null", 4); // Write "null" if m_value is nullptr
        }
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