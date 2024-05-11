#ifndef API_HANDLER_BINANCE_OPEN_ORDERS_H
#define API_HANDLER_BINANCE_OPEN_ORDERS_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

/*  Get all open orders on a symbol
    https://binance-docs.github.io/apidocs/spot/en/#current-open-orders-user_data
*/
class APIHandlerBinanceGetOpenOrders : public APIHandlerBinance
{
public:
    APIHandlerBinanceGetOpenOrders(HttpRequest* request);

    static Json handle_internal_request(const std::string& symbol, const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_get_open_orders_request(const std::string& symbol);
};

#endif //API_HANDLER_BINANCE_OPEN_ORDERS_H