#ifndef API_HANDLER_ADD_ACTIVATE_ACCOUNT_H
#define API_HANDLER_ADD_ACTIVATE_ACCOUNT_H

#include <api_handler/api_handler.h>

class APIHandlerAddActivateAccount : public APIHandler
{
public:
    APIHandlerAddActivateAccount(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_ADD_ACTIVATE_ACCOUNT_H