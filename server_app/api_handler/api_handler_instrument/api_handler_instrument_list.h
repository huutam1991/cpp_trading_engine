#pragma once

#include <api_handler/api_handler.h>

class APIHandlerInstrumentList : public APIHandler
{
public:
    APIHandlerInstrumentList(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
