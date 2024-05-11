#ifndef API_HANDLER_BINANCE_24H_ORDERS_H
#define API_HANDLER_BINANCE_24H_ORDERS_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

class APIHandlerBinance24hOrders : public APIHandlerBinance
{
public:
    APIHandlerBinance24hOrders(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

    Json get_orders_list_in_24h();
    Json get_orders_list_by_time(long from, long to);
};

#endif //API_HANDLER_BINANCE_24H_ORDERS_H