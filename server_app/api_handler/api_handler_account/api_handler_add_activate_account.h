#pragma once

#include <api_handler/api_handler.h>

class APIHandlerAddActivateAccount : public APIHandler
{
public:
    APIHandlerAddActivateAccount(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
