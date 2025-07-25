#pragma once

#include <c_json/json_type_base.h>
#include <c_json/json_value.h>

class JsonNew
{
    JsonTypeBaseNew* m_value = nullptr;

public:
    JsonNew();

    JsonNew(const JsonNew& copy) : m_value{copy.m_value->get_copy()}
    {}

    JsonNew(JsonNew&& copy) : m_value{copy.m_value}
    {
        copy.m_value = nullptr; // Transfer ownership
    }

    JsonNew& operator=(const JsonNew& copy)
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

    JsonNew& operator=(JsonNew&& copy)
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

    ~JsonNew()
    {
        if (m_value != nullptr)
        {
            m_value->release();
            m_value = nullptr;
        }
    }

    const std::string get_string_value() const
    {
        return m_value->get_string_value();
    }

private:
    void check_create_json_value();
    void check_create_json_object();
};