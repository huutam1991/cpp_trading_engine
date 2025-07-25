#include <c_json/json_object.h>

const std::string JsonObjectNew::get_string_value() const
{
    if (m_is_array)
    {
        std::string result = "[";
        for (size_t i = 0; i < m_array.size(); ++i)
        {
            if (i > 0) result += ",";
            result += m_array[i].get_string_value();
        }
        result += "]";
        return result;
    }
    else
    {
        std::string result = "{";
        for (const auto& [key, value] : m_object)
        {
            if (result.size() > 1) result += ",";
            result += "\"" + key + "\":" + value.get_string_value();
        }
        result += "}";
        return result;
    }
}