#pragma once

#include "http_request_post.h"

class HttpRequestPut : public HttpRequestPost
{
public:
    HttpRequestPut(const std::string& content, const std::string& dir_path);

    virtual RequestMethod get_request_method() { return RequestMethod::PUT; }
};

