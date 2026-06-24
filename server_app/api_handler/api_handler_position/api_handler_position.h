#pragma once

#include <api_handler/api_handler.h>

class APIHandlerPosition : public APIHandler
{
public:
    APIHandlerPosition(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
