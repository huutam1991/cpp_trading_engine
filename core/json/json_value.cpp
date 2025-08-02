#include <charconv>
#include <json/json_value.h>

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

void JsonValue::write_string_value(JsonStringBuilder& builder)
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
            auto [ptr, ec] = std::to_chars(buffer_number, buffer_number + sizeof(buffer_number), arg);
            if (ec == std::errc{})
            {
                builder.write_raw(buffer_number, ptr - buffer_number);
            }
        }
        else if constexpr (std::is_same_v<U, ShareString>)
        {
            std::string_view str_view = arg.data();
            if (m_is_string_format)
            {
                builder.write_char('\"');
                builder.write_raw(str_view.data(), str_view.size());
                builder.write_char('\"');
            }
            else
            {
                builder.write_raw(str_view.data(), str_view.size());
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