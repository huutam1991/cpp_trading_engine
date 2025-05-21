#include <json/json_value.h>
#include <json/json.h>

#include <ios>
#include <iomanip>

// const char*
template<>
const std::string JsonValue<const char*>::get_string_value() const
{
    std::string value(m_value);
    if (m_is_string_format == true)
    {
        return std::string("\"" + value + "\"");
    }
    return value;
}

// std::string
template<>
const std::string JsonValue<std::string>::get_string_value() const
{
    if (m_is_string_format == true)
    {
        return std::string("\"" + m_value + "\"");
    }
    return m_value;
}

// bool
template<>
const std::string JsonValue<bool>::get_string_value() const
{
    return std::string(m_value == true ? "true" : "false");
}

// floating point types
template<>
const std::string JsonValue<long double>::get_string_value() const
{
    std::stringstream ss;
    std::string res;
    ss << std::fixed << std::setprecision(10) << m_value;
    ss >> res;
    return res;
}

template<>
const std::string JsonValue<double>::get_string_value() const
{
    std::stringstream ss;
    std::string res;
    ss << std::fixed << std::setprecision(10) << m_value;
    ss >> res;
    return res;
}

template<>
const std::string JsonValue<float>::get_string_value() const
{
    std::stringstream ss;
    std::string res;
    ss << std::fixed << std::setprecision(10) << m_value;
    ss >> res;
    return res;
}

// JsonNull
template<>
const std::string JsonValue<JsonNull>::get_string_value() const
{
    return std::string("null");
}
