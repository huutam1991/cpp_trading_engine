#ifndef HTTP_REQUEST_DELETE_H
#define HTTP_REQUEST_DELETE_H

#include "http_request_post.h"

class HttpRequestDelete : public HttpRequestPost
{
private:

public:
    HttpRequestDelete(const std::string& content, const std::string& dir_path);

    virtual RequestMethod get_request_method() { return RequestMethod::DELETE; }
};

#endif //HTTP_REQUEST_DELETE_H
