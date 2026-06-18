#pragma once

#include <cstddef>

template <std::size_t N>
struct FixedString
{
    char value[N];

    constexpr FixedString(const char (&str)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            value[i] = str[i];
    }

    constexpr operator std::string_view() const
    {
        return {value, N - 1};
    }
};