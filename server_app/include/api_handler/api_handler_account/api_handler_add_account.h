#ifndef API_HANDLER_ADD_ACCOUNT_H
#define API_HANDLER_ADD_ACCOUNT_H

#include <api_handler/api_handler.h>

class APIHandlerAddAccount : public APIHandler
{
public:
    APIHandlerAddAccount(HttpRequest* request);

private:
    virtual Task<HttpResponse> child_handle();
};

#endif //API_HANDLER_ADD_ACCOUNT_H