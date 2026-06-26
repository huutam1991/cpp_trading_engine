#pragma once

#include <api_handler/api_handler.h>

class APIHandlerGatewayList : public APIHandler
{
public:
    APIHandlerGatewayList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
