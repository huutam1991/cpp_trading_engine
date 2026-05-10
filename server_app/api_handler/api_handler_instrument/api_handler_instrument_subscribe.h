#pragma once

#include <api_handler/api_handler.h>

class APIHandlerInstrumentSubscribe : public APIHandler
{
public:
    APIHandlerInstrumentSubscribe(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
