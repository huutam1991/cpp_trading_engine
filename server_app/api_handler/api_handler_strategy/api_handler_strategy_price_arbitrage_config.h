#pragma once

#include <api_handler/api_handler.h>

class APIHandlerStrategyPAConfig : public APIHandler
{
public:
    APIHandlerStrategyPAConfig(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
