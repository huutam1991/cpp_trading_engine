#pragma once

#include <api_handler/api_handler.h>

class APIHandlerSetActiveAccount : public APIHandler
{
public:
    APIHandlerSetActiveAccount(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
