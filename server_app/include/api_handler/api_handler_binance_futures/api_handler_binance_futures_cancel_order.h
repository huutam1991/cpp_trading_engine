#ifndef API_HANDLER_BINANCE_FUTURES_CANCEL_ORDER_H
#define API_HANDLER_BINANCE_FUTURES_CANCEL_ORDER_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

/*  Cancel an active order.
    https://binance-docs.github.io/apidocs/futures/en/#cancel-order-trade
*/
class APIHandlerBinanceFuturesCancelOrder : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFuturesCancelOrder(HttpRequest* request);

    static Json handle_internal_request(const std::string& symbol, long orderId);

private:
    virtual HttpResponse child_handle();

    Json send_cancel_order_request(const std::string& symbol, long orderId);
};

#endif //API_HANDLER_BINANCE_FUTURES_CANCEL_ORDER_H