#ifndef HTTP_REQUEST_PUT_H
#define HTTP_REQUEST_PUT_H

#include "http_request_post.h"

class HttpRequestPut : public HttpRequestPost
{
public:
    HttpRequestPut(const std::string& content, const std::string& dir_path);

    virtual RequestMethod get_request_method() { return RequestMethod::PUT; }
};

#endif //HTTP_REQUEST_PUT_H
