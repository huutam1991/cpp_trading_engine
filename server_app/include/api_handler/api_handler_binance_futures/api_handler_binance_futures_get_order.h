
#ifndef API_HANDLER_BINANCE_FUTURES_GET_ORDER_H
#define API_HANDLER_BINANCE_FUTURES_GET_ORDER_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

/*  Check an order's status.
    https://binance-docs.github.io/apidocs/futures/en/#query-order-user_data
*/
class APIHandlerBinanceFuturesGetOrder : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFuturesGetOrder(HttpRequest* request);

    static Json handle_internal_request(const std::string& symbol, long orderId, const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_get_order_info_request(const std::string& symbol, long orderId);
};

#endif //API_HANDLER_BINANCE_FUTURES_GET_ORDER_H