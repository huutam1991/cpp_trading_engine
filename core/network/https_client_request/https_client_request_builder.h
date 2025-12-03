#pragma once

#include <string>

struct RequestBuilder
{
    char buf[2048];
    size_t len = 0;

    template<typename T>
    inline void append(const T& s)
    {
        if constexpr (requires { s.size(); })
        {
            size_t n = s.size();
            memcpy(buf + len, s.data(), n);
            len += n;
        }
        else
        {
            constexpr size_t N = sizeof(T);
            memcpy(buf + len, s, N - 1);
            len += (N - 1);
        }
    }

    inline std::string to_string()
    {
        return std::string(buf, len);
    }
};