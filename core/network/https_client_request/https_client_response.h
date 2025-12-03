#pragma once

#include <string>
#include <sstream>
#include <strings.h>
#include <unordered_map>

struct HttpsClientResponse
{
    int status_code = 0;
    std::string status_message;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    bool is_complete = false;
};