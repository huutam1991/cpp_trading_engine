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
