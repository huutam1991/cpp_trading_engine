#include <json/json_object.h>


void JsonObject::for_each(std::function<void(Json&)>& loop_func)
{
    // Loop through object
    if (m_is_array == false)
    {
        for (auto it = m_object.begin(); it != m_object.end(); it++)
        {
            loop_func(it->second);
        }
    }
    // Loop through array
    else
    {
        for (auto it = m_array.begin(); it != m_array.end(); it++)
        {
            loop_func(*it);
        }
    }
}

void JsonObject::for_each_with_key(std::function<void(const std::string&, Json&)>& loop_func)
{
    if (m_is_array == false)
    {
        // Loop through object
        for (auto it = m_object.begin(); it != m_object.end(); it++)
        {
            loop_func(it->first, it->second);
        }
    }
}

void JsonObject::for_each_with_index(std::function<void(size_t,Json&)>& loop_func)
{
    if (m_is_array == true)
    {
        for (size_t i = 0; i < m_array.size(); i++)
        {
            loop_func(i, m_array[i]);
        }
    }
}

void JsonObject::write_string_value(JsonStringBuilder& builder)
{
    if (m_is_array)
    {
        builder.write_char('[');
        for (size_t i = 0; i < m_array.size(); ++i)
        {
            if (i > 0) builder.write_char(',');
            m_array[i].write_string_value(builder);
        }
        builder.write_char(']');
    }
    else
    {
        std::string result = "{";
        builder.write_char('{');
        int count = 0;
        for (const auto& [key, value] : m_object)
        {
            if (count++ > 0) builder.write_char(',');
            builder.write_char('\"');
            builder.write_raw(key.data(), key.size());
            builder.write_char('\"');
            builder.write_char(':');
            value.write_string_value(builder);
        }
        builder.write_char('}');
    }
}

JsonTypeBase* JsonObject::get_deep_clone()
{
    JsonObject* clone = JsonObjectPool::acquire();
    clone->m_is_array = m_is_array;

    // Deep clone the array
    clone->m_array.reserve(m_array.size());
    for (auto& item : m_array)
    {
        clone->m_array.push_back(item.deep_clone());
    }

    // Deep clone the object
    for (auto& [key, value] : m_object)
    {
        clone->m_object[key] = value.deep_clone();
    }

    return clone;
}