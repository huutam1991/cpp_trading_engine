#pragma once

#include <api_handler/api_handler.h>

class APIHandlerStrategyConfig : public APIHandler
{
public:
    APIHandlerStrategyConfig(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
