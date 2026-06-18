#pragma once

#include <api_handler/api_handler.h>

class APIHandlerFlowMetric : public APIHandler
{
public:
    APIHandlerFlowMetric(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
