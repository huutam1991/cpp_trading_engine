#pragma once

#include <array>

#define STRING_FIXED_SIZE 20000 // Reserve space for 20000 characters

class FixString
{
    std::array<char, STRING_FIXED_SIZE> m_data;

public:
    FixString() = default;
    ~FixString() = default;
    FixString(const FixString&) = delete;
    FixString(FixString&&) = default;
    FixString& operator=(const FixString&) = delete;
    FixString& operator=(FixString&&) = delete;

    char* data()
    {
        return m_data.data();
    }
};