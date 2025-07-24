#pragma once

#include <c_json/json_type_base.h>

class JsonNew
{
    JsonTypeBase* m_value = nullptr;

public:
    JsonNew();
    JsonNew(const JsonNew& copy)
    {
        m_value = copy.m_value->get_copy();
    }

    JsonNew& operator=(const JsonNew&) = delete;


    JsonNew(JsonNew&&) = delete;
    JsonNew& operator=(JsonNew&&) = delete;

    ~JsonNew()
    {
        if (m_value)
        {
            m_value->release();
        }
    }

    const std::string get_string_value() const
    {
        return m_value->get_string_value();
    }

    // Other methods...
};