#pragma once

#include <api_handler/api_handler.h>

class APIHandlerExchangeIdList : public APIHandler
{
public:
    APIHandlerExchangeIdList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
