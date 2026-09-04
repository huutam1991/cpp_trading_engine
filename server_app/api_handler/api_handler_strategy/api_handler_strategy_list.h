#pragma once

#include <api_handler/api_handler.h>

class APIHandlerStrategyList : public APIHandler
{
public:
    APIHandlerStrategyList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
