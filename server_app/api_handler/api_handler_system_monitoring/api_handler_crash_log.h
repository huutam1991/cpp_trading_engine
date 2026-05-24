#pragma once

#include <api_handler/api_handler.h>

class APIHandlerCrashLog : public APIHandler
{
public:
    APIHandlerCrashLog(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
