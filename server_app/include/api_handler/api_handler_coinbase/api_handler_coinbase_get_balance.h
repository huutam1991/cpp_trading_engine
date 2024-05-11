#ifndef API_HANDLER_COINBASE_GET_BALANCE_H
#define API_HANDLER_COINBASE_GET_BALANCE_H

#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>

class APIHandlerCoinbaseGetBalance : public APIHandlerCoinbase
{
public:
    APIHandlerCoinbaseGetBalance(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_COINBASE_GET_BALANCE_H