#include <c_json/json.h>
#include <c_json/json_object.h>
#include <c_json/json_value.h>

JsonNew::JsonNew()
{}

JsonNew& JsonNew::operator[](const char* key)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    return (*json_object)[key];
}

JsonNew& JsonNew::operator[](size_t index)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    return (*json_object)[index];
}

void JsonNew::check_create_json_value()
{
    if (m_value == nullptr)
    {
        m_value = JsonValuePool::acquire();
    }

    if (m_value->is_json_value() == false)
    {
        m_value->release();
        m_value = JsonValuePool::acquire();
    }
}

void JsonNew::check_create_json_object()
{
    if (m_value == nullptr)
    {
        m_value = JsonObjectPool::acquire();
    }

    if (m_value->is_json_value() == true)
    {
        m_value->release();
        m_value = JsonObjectPool::acquire();
    }
}