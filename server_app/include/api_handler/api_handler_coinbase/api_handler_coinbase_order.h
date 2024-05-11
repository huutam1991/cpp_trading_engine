#ifndef API_HANDLER_COINBASE_ORDER_H
#define API_HANDLER_COINBASE_ORDER_H

#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>

class APIHandlerCoinbaseOrder : public APIHandlerCoinbase
{
public:
    APIHandlerCoinbaseOrder(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_COINBASE_ORDER_H