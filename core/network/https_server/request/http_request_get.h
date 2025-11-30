#pragma once

#include "http_request.h"

class HttpRequestGet : public HttpRequest
{
public:
    HttpRequestGet(const std::string& content, const std::string& dir_path);

    virtual RequestMethod get_request_method() { return RequestMethod::GET; }
    virtual std::string get_body() { return std::string(""); }
};

