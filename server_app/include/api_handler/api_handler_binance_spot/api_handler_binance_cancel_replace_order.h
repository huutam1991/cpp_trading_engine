#ifndef API_HANDLER_BINANCE_CANCEL_REPLACE_ORDER_H
#define API_HANDLER_BINANCE_CANCEL_REPLACE_ORDER_H

#include <api_handler/api_handler_binance_spot/api_handler_binance_create_order.h>

/*  Cancels an existing order and places a new order on the same symbol.
    Filters and Order Count are evaluated before the processing of the cancellation and order placement occurs.
    A new order that was not attempted (i.e. when newOrderResult: NOT_ATTEMPTED), will still increase the order count by 1.
    https://binance-docs.github.io/apidocs/spot/en/#cancel-an-existing-order-and-send-a-new-order-trade
*/
class APIHandlerBinanceCancelReplaceOrder : public APIHandlerBinanceCreateOrder
{
public:
    APIHandlerBinanceCancelReplaceOrder(HttpRequest* request);

    static Json handle_internal_request(Json& query_json, const std::string user_id = "root");

protected:
    virtual Json send_new_order_request(Json& query_json);
    virtual std::string get_query(Json& query_json);
};

#endif //API_HANDLER_BINANCE_CANCEL_REPLACE_ORDER_H