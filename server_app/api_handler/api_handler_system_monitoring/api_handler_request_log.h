#pragma once

#include <api_handler/api_handler.h>

class APIHandlerRequestLog : public APIHandler
{
public:
    APIHandlerRequestLog(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
