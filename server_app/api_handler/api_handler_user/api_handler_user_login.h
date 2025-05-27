#pragma once

#include <api_handler/api_handler.h>

class APIHandlerUserLogin : public APIHandler
{
public:
    APIHandlerUserLogin(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
