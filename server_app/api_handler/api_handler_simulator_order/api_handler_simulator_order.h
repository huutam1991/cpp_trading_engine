#pragma once

#include <api_handler/api_handler.h>

class APIHandlerSimulatorOrder : public APIHandler
{
public:
    APIHandlerSimulatorOrder(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
