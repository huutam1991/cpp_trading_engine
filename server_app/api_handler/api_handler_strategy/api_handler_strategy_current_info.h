#pragma once

#include <api_handler/api_handler.h>

class APIHandlerStrategyCurrentInfo : public APIHandler
{
public:
    APIHandlerStrategyCurrentInfo(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
