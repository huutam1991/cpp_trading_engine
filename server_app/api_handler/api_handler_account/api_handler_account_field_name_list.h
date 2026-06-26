#pragma once

#include <api_handler/api_handler.h>

class APIHandlerAccountFieldNameList : public APIHandler
{
public:
    APIHandlerAccountFieldNameList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
