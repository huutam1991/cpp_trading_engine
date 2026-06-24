#pragma once

#include <api_handler/api_handler.h>

class APIHandlerPositionList : public APIHandler
{
public:
    APIHandlerPositionList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
