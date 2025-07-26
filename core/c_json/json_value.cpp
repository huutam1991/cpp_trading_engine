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

void JsonValueNew::write_string_value(JsonStringBuilder& builder) const
{
    std::visit([this, &builder](auto&& arg) -> void
    {
        using U = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<U, std::nullptr_t>)
        {
            builder.write_raw("null", 4);
        }
        else if constexpr (std::is_same_v<U, bool>)
        {
            if (arg)
            {
                builder.write_raw("true", 4);
            }
            else
            {
                builder.write_raw("false", 5);
            }
        }
        else if constexpr (std::is_arithmetic_v<U>)
        {
            std::string number = std::to_string(arg);
            builder.write_raw(number.c_str(), number.size());
        }
        else if constexpr (std::is_same_v<U, ShareString>)
        {
            if (m_is_string_format)
            {
                builder.write_char('\"');
                builder.write_raw(arg.data().data(), arg.data().size());
                builder.write_char('\"');
            }
            else
            {
                builder.write_raw(arg.data().data(), arg.data().size());
            }
        }
        else if constexpr (std::is_same_v<U, std::string_view>)
        {
            if (m_is_string_format)
            {
                builder.write_char('\"');
                builder.write_raw(arg.data(), arg.size());
                builder.write_char('\"');
            }
            else
            {
                builder.write_raw(arg.data(), arg.size());
            }
        }
        else if constexpr (std::is_same_v<U, const char*>)
        {
            if (m_is_string_format)
            {
                builder.write_char('\"');
                builder.write_raw(arg, std::strlen(arg));
                builder.write_char('\"');
            }
            else
            {
                builder.write_raw(arg, std::strlen(arg));
            }
        }
        else
        {
            builder.write_raw("<unsupported>", 13);
        }
    }, m_value);
}