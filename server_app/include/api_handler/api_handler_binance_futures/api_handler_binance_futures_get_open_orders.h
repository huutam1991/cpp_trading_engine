
#ifndef API_HANDLER_BINANCE_FUTURES_GET_OPEN_ORDERS_H
#define API_HANDLER_BINANCE_FUTURES_GET_OPEN_ORDERS_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

/*  Get all open orders on a symbol
    https://binance-docs.github.io/apidocs/futures/en/#current-all-open-orders-user_data
*/
class APIHandlerBinanceFuturesGetOpenOrders : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFuturesGetOpenOrders(HttpRequest* request);

    static Json handle_internal_request(const std::string& symbol, const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_get_open_orders_request(const std::string& symbol);
};

#endif //API_HANDLER_BINANCE_FUTURES_GET_OPEN_ORDERS_H