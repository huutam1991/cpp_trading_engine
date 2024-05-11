#ifndef API_HANDLER_BINANCE_ORDER_H
#define API_HANDLER_BINANCE_ORDER_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

class APIHandlerBinanceOrder : public APIHandlerBinance
{
public:
    APIHandlerBinanceOrder(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

    std::string get_open_order_has_same_quantity_and_price();
    Json get_open_order_has_same_price();
    Json send_new_order_request();
};

#endif //API_HANDLER_BINANCE_ORDER_H