#pragma once

#include <api_handler/api_handler.h>

class APIHandlerOrderList : public APIHandler
{
public:
    APIHandlerOrderList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
