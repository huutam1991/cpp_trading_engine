#ifndef API_HANDLER_BINANCE_CANCEL_ORDER_H
#define API_HANDLER_BINANCE_CANCEL_ORDER_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

/*  Cancel an active order.
    https://binance-docs.github.io/apidocs/spot/en/#cancel-order-trade
*/
class APIHandlerBinanceCancelOrder : public APIHandlerBinance
{
public:
    APIHandlerBinanceCancelOrder(HttpRequest* request);

    static Json handle_internal_request(const std::string& symbol, long orderId, const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_cancel_order_request(const std::string& symbol, long orderId);
};

#endif //API_HANDLER_BINANCE_CANCEL_ORDER_H