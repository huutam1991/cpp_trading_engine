#include <string>
#include <iostream>
#include <charconv>

#include <json/json_parse.h>

namespace
{
    inline bool safe_is_number(char c)
    {
        return JsonParseNew::is_number(c);
    }

    inline int safe_is_boolean(const std::string_view& s, size_t pos)
    {
        return JsonParseNew::is_boolean(s.data(), pos, s.size());
    }

    inline bool safe_is_null(const std::string_view& s, size_t pos)
    {
        return JsonParseNew::is_null(s.data(), pos, s.size());
    }
}

ShareString JsonParseNew::get_sub_string(size_t start, size_t end)
{
    if (end < start)
    {
        m_is_valid = false;
        return {};
    }

    size_t size = end - start;
    return ShareString(m_share_string.from_substr(start, size));
}

JsonParseNew::JsonParseNew(std::string object) : m_share_string(std::move(object))
{}

Json JsonParseNew::parse()
{
    Json res;

    m_object_string = m_share_string.data();
    m_size = m_object_string.size();
    m_is_valid = true;

    if (m_size == 0)
    {
        res = nullptr;
        return res;
    }

    size_t start = 0;
    while (start < m_size && m_object_string[start] != '{' && m_object_string[start] != '[')
    {
        ++start;
    }

    if (start >= m_size)
    {
        m_is_valid = false;
        res = nullptr;
        return res;
    }

    const char opening_char = m_object_string[start];
    ++start;

    res = opening_char == '{' ? parse_object(start) : parse_array(start);
    if (m_is_valid == false)
    {
        res = nullptr;
    }

    return res;
}

bool JsonParseNew::check_exceed_size(size_t index)
{
    if (index >= m_size)
    {
        m_is_valid = false;
        return true;
    }

    return false;
}

Json JsonParseNew::parse_object(size_t& start_pos)
{
    Json res;

    while (start_pos < m_size)
    {
        if (m_object_string[start_pos] == '}')
        {
            ++start_pos;
            return res;
        }

        std::string key = parse_key(start_pos);
        if (m_is_valid == false)
        {
            return Json();
        }

        if (key == "}")
        {
            return res;
        }

        Json value = parse_value(start_pos);
        if (m_is_valid == false)
        {
            return Json();
        }

        res[key] = value;

        if (start_pos < m_size && m_object_string[start_pos] == ',')
        {
            ++start_pos;
        }
    }

    m_is_valid = false;
    return Json();
}

Json JsonParseNew::parse_array(size_t& start_pos)
{
    Json res;
    int index = 0;

    while (start_pos < m_size)
    {
        if (m_object_string[start_pos] == ']')
        {
            ++start_pos;
            return res;
        }

        Json value = parse_value(start_pos);
        if (m_is_valid == false)
        {
            return Json();
        }

        res[index++] = value;

        if (start_pos < m_size && m_object_string[start_pos] == ',')
        {
            ++start_pos;
        }
    }

    m_is_valid = false;
    return Json();
}

std::string JsonParseNew::parse_key(size_t& start_pos)
{
    size_t start = start_pos;
    while (start < m_size && m_object_string[start] != '"' && m_object_string[start] != '}')
    {
        ++start;
    }

    if (start >= m_size)
    {
        m_is_valid = false;
        return {};
    }

    if (m_object_string[start] == '}')
    {
        start_pos = start + 1;
        return "}";
    }

    ++start;

    size_t end = start;
    while (end < m_size && m_object_string[end] != '"')
    {
        ++end;
    }

    if (end >= m_size)
    {
        m_is_valid = false;
        return {};
    }

    start_pos = end + 1;
    return std::string(get_sub_string(start, end).data());
}

Json JsonParseNew::parse_value(size_t& start_pos)
{
    Json res;
    size_t start = start_pos;

    while (start < m_size)
    {
        const char current = m_object_string[start];

        if (current == '"' || current == '{' || current == '[' ||
            safe_is_number(current) ||
            safe_is_boolean(m_object_string, start) != NOT_BOOLEAN_VALUE ||
            safe_is_null(m_object_string, start))
        {
            break;
        }

        if (current == '}' || current == ']')
        {
            start_pos = start;
            return res;
        }

        ++start;
    }

    if (start >= m_size)
    {
        m_is_valid = false;
        return Json();
    }

    if (m_object_string[start] == '"')
    {
        start_pos = start + 1;
        res = parse_value_string(start_pos);
    }
    else if (safe_is_number(m_object_string[start]))
    {
        start_pos = start;
        res = parse_value_number(start_pos);
    }
    else
    {
        const int bool_value = safe_is_boolean(m_object_string, start);
        if (bool_value == TRUE_VALUE)
        {
            res = true;
            start_pos = start + 4;
        }
        else if (bool_value == FALSE_VALUE)
        {
            res = false;
            start_pos = start + 5;
        }
        else if (safe_is_null(m_object_string, start))
        {
            res = nullptr;
            start_pos = start + 4;
        }
        else if (m_object_string[start] == '{')
        {
            start_pos = start + 1;
            res = parse_object(start_pos);
        }
        else if (m_object_string[start] == '[')
        {
            start_pos = start + 1;
            res = parse_array(start_pos);
        }
        else
        {
            m_is_valid = false;
            return Json();
        }
    }

    while (start_pos < m_size && m_object_string[start_pos] != ',' &&
           m_object_string[start_pos] != '}' && m_object_string[start_pos] != ']')
    {
        ++start_pos;
    }

    if (start_pos >= m_size)
    {
        m_is_valid = false;
    }

    return res;
}

ShareString JsonParseNew::parse_value_string(size_t& start_pos)
{
    size_t start = start_pos;
    size_t end = start_pos;

    while (end < m_size && m_object_string[end] != '"')
    {
        ++end;
    }

    if (end >= m_size)
    {
        m_is_valid = false;
        return {};
    }

    start_pos = end + 1;
    return get_sub_string(start, end);
}

Json JsonParseNew::parse_value_number(size_t& start_pos)
{
    Json res;
    size_t start = start_pos;
    bool is_float = false;
    size_t end = start_pos;

    while (end < m_size)
    {
        const char current = m_object_string[end];
        const bool has_negative_exponent =
            current == 'e' && (end + 1) < m_size && m_object_string[end + 1] == '-';

        if (!(safe_is_number(current) || current == '.' || has_negative_exponent))
        {
            break;
        }

        if (current == '.')
        {
            is_float = true;
        }

        ++end;
    }

    if (end == start)
    {
        m_is_valid = false;
        return Json();
    }

    start_pos = end;

    std::string_view number_string = get_sub_string(start, end).data();
    if (number_string.empty())
    {
        m_is_valid = false;
        return Json();
    }

    if (is_float == false)
    {
        int64_t int_number = 0;
        auto [ptr, ec] = std::from_chars(number_string.data(), number_string.data() + number_string.size(), int_number);
        if (ec == std::errc())
        {
            res = int_number;
        }
        else
        {
            m_is_valid = false;
        }
    }
    else
    {
        double double_number = 0.0;
        auto [ptr, ec] = std::from_chars(number_string.data(), number_string.data() + number_string.size(), double_number);
        if (ec == std::errc())
        {
            res = double_number;
        }
        else
        {
            m_is_valid = false;
        }
    }

    return res;
}
