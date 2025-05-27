#pragma once

#include <api_handler/api_handler.h>

class APIHandlerActivateAccountBalances : public APIHandler
{
public:
    APIHandlerActivateAccountBalances(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};
