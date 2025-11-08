#include <charconv>
#include <json/json_value.h>

JsonValue::JsonValue()
{
    is_json_value_ptr = static_cast<IsJsonValuePtr>(&JsonValue::is_json_value);
}

template<>
JsonValue::operator std::string() const
{
    if (std::holds_alternative<ShareString>(m_value))
    {
        std::string_view str_view = std::get<ShareString>(m_value).data();
        return std::string(str_view);
    }
    else
    {
        return {};
    }
}