#include <c_json/json.h>
#include <c_json/json_object.h>
#include <c_json/json_value.h>

JsonNew::JsonNew()
{}

JsonNew::JsonNew(std::initializer_list<std::pair<std::string, JsonNew>> json_list)
{
    check_create_json_object(); // Create JsonObject if it does not exist
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;

    for (auto& pair : json_list)
    {
        json_object->add_pair(pair);
    }
}

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

template<>
bool JsonNew::operator ==(const char* value) const
{
    if (m_value == nullptr || m_value->is_json_value() == false)
    {
        return false; // If m_value is null or not a JsonValue, return false
    }
    else
    {
        ShareString current_value = ((JsonValueNew*)m_value)->operator ShareString();
        std::string_view current_value_view = current_value.data();
        std::string_view value_view(value);
        return current_value_view == value_view;
    }
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

bool JsonNew::has_field(const std::string& field) const
{
    if (m_value->is_json_value() == false)
    {
        return ((JsonObjectNew*)m_value)->has_field(field);
    }
    return false; // If it's a JsonValue, it cannot have fields
}

void JsonNew::remove_field(const std::string& field)
{
    if (m_value->is_json_value() == false)
    {
        ((JsonObjectNew*)m_value)->remove_field(field);
    }
}

void JsonNew::set_size(size_t size)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    json_object->set_size(size);
}

int JsonNew::size() const
{
    return m_value->is_json_value() ? 0 : ((JsonObjectNew*)m_value)->size();
}

void JsonNew::reverse()
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    json_object->reverse();
}

void JsonNew::sort(std::function<bool(JsonNew&, JsonNew&)> compare_func)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    json_object->sort(compare_func);
}

void JsonNew::push_back(const JsonNew& value)
{
    check_create_json_object();
    JsonObjectNew* json_object = (JsonObjectNew*)m_value;
    json_object->push_back(value);
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