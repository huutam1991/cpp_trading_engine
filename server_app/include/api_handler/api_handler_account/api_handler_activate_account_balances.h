#ifndef API_HANDLER_ACTIVATE_ACCOUNT_BALANCES_H
#define API_HANDLER_ACTIVATE_ACCOUNT_BALANCES_H

#include <api_handler/api_handler.h>

class APIHandlerActivateAccountBalances : public APIHandler
{
public:
    APIHandlerActivateAccountBalances(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_ACTIVATE_ACCOUNT_BALANCES_H