#include <c_json/json_object.h>

void JsonObjectNew::write_string_value(JsonStringBuilder& builder) const
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