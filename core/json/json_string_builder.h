#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <fstream>

#include <spdlog/spdlog.h>

// Write to a std::string
class JsonStringBuilder
{
    char* m_ptr;
    size_t pos = 0;

public:
    JsonStringBuilder(char* ptr) : m_ptr(ptr)
    {}

    virtual inline void write_char(char c)
    {
        m_ptr[pos++] = c;
    }

    virtual inline void write_raw(const char* data, size_t len)
    {
        memcpy(m_ptr + pos, data, len);
        pos += len;
    }

    inline std::string finish()
    {
        return std::string(m_ptr, pos);
    }
};

// Write to a std::ofstream (file)
class JsonFileWriter : public JsonStringBuilder
{
    std::ofstream& out;

public:
    JsonFileWriter(std::ofstream& o) : JsonStringBuilder(nullptr), out(o) {}

    virtual inline void write_char(char c)
    {
        out.put(c);
    }

    virtual inline void write_raw(const char* data, size_t len)
    {
        out.write(data, len);
    }

    inline void finish()
    {
        out.flush();
    }
};