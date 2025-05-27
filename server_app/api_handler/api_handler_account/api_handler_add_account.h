#pragma once

#include <api_handler/api_handler.h>

class APIHandlerAddAccount : public APIHandler
{
public:
    APIHandlerAddAccount(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
