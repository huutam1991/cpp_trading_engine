#pragma once

#include <api_handler/api_handler.h>

class APIHandlerStrategyPACurrentInfo : public APIHandler
{
public:
    APIHandlerStrategyPACurrentInfo(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
