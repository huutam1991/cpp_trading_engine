#pragma once

#include <api_handler/api_handler.h>

class APIHandlerUpTime : public APIHandler
{
public:
    APIHandlerUpTime(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
