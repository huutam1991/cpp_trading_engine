#pragma once

#include <string>

#define STRING_FIXED_SIZE 10000 // Reserve space for 10000 characters

class FixString
{
    std::string m_string;

public:
    FixString()
    {
        m_string.reserve(STRING_FIXED_SIZE);
    }

    char* data()
    {
        return m_string.data();
    }
};