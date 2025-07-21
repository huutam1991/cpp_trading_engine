#include <c_json/json_value.h>

template<>
JsonValue& JsonValue::operator=(const std::string& value)
{
    m_value = ShareString(value);
    return *this;
}

template<>
JsonValue& JsonValue::operator=(std::string&& value)
{
    m_value = ShareString(std::move(value));
    return *this;
}

template<>
JsonValue::operator std::string_view()
{
    if (std::holds_alternative<ShareString>(m_value))
    {
        return std::get<ShareString>(m_value).data();
    }
    else if (std::holds_alternative<std::string_view>(m_value))
    {
        return std::get<std::string_view>(m_value);
    }
    else if (std::holds_alternative<const char*>(m_value))
    {
        return std::string_view(std::get<const char*>(m_value));
    }
    else
    {
        return {};
    }
}

template<>
JsonValue::operator std::string()
{
    if (std::holds_alternative<ShareString>(m_value))
    {
        std::string_view str_view = std::get<ShareString>(m_value).data();
        return std::string(str_view);
    }
    else if (std::holds_alternative<std::string_view>(m_value))
    {
        return std::string(std::get<std::string_view>(m_value));
    }
    else if (std::holds_alternative<const char*>(m_value))
    {
        return std::string(std::get<const char*>(m_value));
    }
    else
    {
        return {};
    }
}