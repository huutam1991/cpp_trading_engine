
#ifndef API_HANDLER_BINANCE_FUTURES_CREATE_STOP_ORDER_H
#define API_HANDLER_BINANCE_FUTURES_CREATE_STOP_ORDER_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_create_order.h>

class APIHandlerBinanceFuturesCreateStopOrder : public APIHandlerBinanceFuturesCreateOrder
{
public:
    APIHandlerBinanceFuturesCreateStopOrder(HttpRequest* request);

    static Json handle_internal_request(const std::string& user_id, Json& query_json, Json& price_ticker);

protected:
    virtual std::string get_query_string(Json& query_json);
};

#endif //API_HANDLER_BINANCE_FUTURES_CREATE_STOP_ORDER_H