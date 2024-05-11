#ifndef API_HANDLER_BINANCE_FUTURES_CREATE_ORDER_H
#define API_HANDLER_BINANCE_FUTURES_CREATE_ORDER_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

/*  Send in a new order.
    https://binance-docs.github.io/apidocs/futures/en/#new-order-trade
*/
class APIHandlerBinanceFuturesCreateOrder : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFuturesCreateOrder(HttpRequest* request);

    static Json handle_internal_request(Json& query_json);

protected:
    virtual HttpResponse child_handle();
    virtual Json send_new_order_request(Json& query_json);

    virtual std::string get_query_string(Json& query_json);

    void handle_query_json(Json& query_json);
    Json handle_send_order_request(Json& query_json);
};

#endif //API_HANDLER_BINANCE_FUTURES_CREATE_ORDER_H