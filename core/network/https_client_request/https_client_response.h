#pragma once

#include <string>
#include <unordered_map>

struct HttpsClientResponse
{
    int status_code = 0;
    std::string status_message;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    bool is_complete = false;

    void reset()
    {
        status_code = 0;
        status_message.clear();
        headers.clear();
        body.clear();
        is_complete = false;
    }

    static HttpsClientResponse create_error_response()
    {
        return HttpsClientResponse {-1, "Disconnected", {}, "", false};
    }

    static inline std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t");
        size_t end   = s.find_last_not_of(" \t");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }

    static inline std::string to_lower(std::string s)
    {
        for (char& c : s) c = (char)std::tolower((unsigned char)c);
        return s;
    }
};