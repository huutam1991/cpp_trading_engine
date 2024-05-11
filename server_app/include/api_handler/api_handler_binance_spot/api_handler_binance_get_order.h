#ifndef API_HANDLER_BINANCE_GET_ORDER_H
#define API_HANDLER_BINANCE_GET_ORDER_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

/*  Check an order's status.
    https://binance-docs.github.io/apidocs/spot/en/#query-order-user_data
*/
class APIHandlerBinanceGetOrder : public APIHandlerBinance
{
public:
    APIHandlerBinanceGetOrder(HttpRequest* request);

    static Json handle_internal_request(const std::string& symbol, long orderId, const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_get_order_info_request(const std::string& symbol, long orderId);
};

#endif //API_HANDLER_BINANCE_GET_ORDER_H