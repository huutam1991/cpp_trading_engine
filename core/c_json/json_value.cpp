#include <c_json/json_value.h>

template<>
JsonValueNew& JsonValueNew::operator=(const std::string& value)
{
    m_value = ShareString(value);
    return *this;
}

template<>
JsonValueNew& JsonValueNew::operator=(std::string&& value)
{
    m_value = ShareString(std::move(value));
    return *this;
}

template<>
JsonValueNew::operator std::string() const
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

std::string JsonValueNew::get_string_value() const
{
    return std::visit([this](auto&& arg) -> std::string
    {
        using U = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<U, std::nullptr_t>)
        {
            return "null";
        }
        else if constexpr (std::is_same_v<U, bool>)
        {
            return arg ? "true" : "false";
        }
        else if constexpr (std::is_arithmetic_v<U>)
        {
            return std::to_string(arg);
        }
        else if constexpr (std::is_same_v<U, ShareString>)
        {
            if (m_is_string_format)
            {
                return "\"" + std::string(arg.data()) + "\"";
            }
            return std::string(arg.data());
        }
        else if constexpr (std::is_same_v<U, std::string_view>)
        {
            if (m_is_string_format)
            {
                return "\"" + std::string(arg) + "\"";
            }
            return std::string(arg);
        }
        else if constexpr (std::is_same_v<U, const char*>)
        {
            if (m_is_string_format)
            {
                return "\"" + std::string(arg) + "\"";
            }
            return std::string(arg);
        }
        else
        {
            return "<unsupported>";
        }
    }, m_value);
}