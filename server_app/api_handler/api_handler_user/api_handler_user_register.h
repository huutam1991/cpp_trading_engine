#pragma once

#include <api_handler/api_handler.h>

class APIHandlerUserRegister : public APIHandler
{
public:
    APIHandlerUserRegister(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
