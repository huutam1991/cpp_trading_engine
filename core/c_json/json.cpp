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

JsonNew& JsonNew::operator[](const std::string& key)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    return (*json_object)[key.c_str()];
}

JsonNew& JsonNew::operator[](size_t index)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    return (*json_object)[index];
}

void JsonNew::for_each(std::function<void(JsonNew&)> loop_func)
{
    if (m_value->is_json_value() == false)
    {
        ((JsonObjectNew*)m_value)->for_each(loop_func);
    }
}

void JsonNew::for_each_with_key(std::function<void(const std::string&,JsonNew&)> loop_func)
{

    if (m_value->is_json_value() == false)
    {
        ((JsonObjectNew*)m_value)->for_each_with_key(loop_func);
    }
}

void JsonNew::for_each_with_index(std::function<void(size_t,JsonNew&)> loop_func)
{
    if (m_value->is_json_value() == false)
    {
        ((JsonObjectNew*)m_value)->for_each_with_index(loop_func);
    }
}

bool JsonNew::is_null() const
{
    return m_value == nullptr || (m_value->is_json_value() && ((JsonValueNew*)m_value)->is_null());
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