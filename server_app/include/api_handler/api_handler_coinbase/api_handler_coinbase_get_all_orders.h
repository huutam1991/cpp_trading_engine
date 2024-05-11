#ifndef API_HANDLER_COINBASE_GET_ALL_ORDERS_H
#define API_HANDLER_COINBASE_GET_ALL_ORDERS_H

#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>

class APIHandlerCoinbaseGetAllOrders : public APIHandlerCoinbase
{
public:
    APIHandlerCoinbaseGetAllOrders(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_COINBASE_GET_ALL_ORDERS_H