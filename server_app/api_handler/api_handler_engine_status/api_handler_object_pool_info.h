#pragma once

#include <api_handler/api_handler.h>

class APIHandlerObjectPoolInfo : public APIHandler
{
public:
    APIHandlerObjectPoolInfo(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
