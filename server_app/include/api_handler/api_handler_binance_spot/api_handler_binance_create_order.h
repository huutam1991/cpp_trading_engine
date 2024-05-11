#ifndef API_HANDLER_BINANCE_CREATE_ORDER_H
#define API_HANDLER_BINANCE_CREATE_ORDER_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

/*  Send in a new order.
    https://binance-docs.github.io/apidocs/spot/en/#new-order-trade
*/
class APIHandlerBinanceCreateOrder : public APIHandlerBinance
{
public:
    APIHandlerBinanceCreateOrder(HttpRequest* request);

    static Json handle_internal_request(Json& query_json, const std::string user_id = "root");

protected:
    virtual HttpResponse child_handle();
    virtual Json send_new_order_request(Json& query_json);

    void handle_query_json(Json& query_json);
    Json handle_send_order_request(Json& query_json);
};

#endif //API_HANDLER_BINANCE_CREATE_ORDER_H