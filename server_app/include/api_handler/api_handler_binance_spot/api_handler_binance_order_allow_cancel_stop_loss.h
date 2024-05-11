#ifndef API_HANDLER_BINANCE_ORDER_ALLOW_CANCEL_STOP_LOSS_H
#define API_HANDLER_BINANCE_ORDER_ALLOW_CANCEL_STOP_LOSS_H

#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_replace_order.h>

class APIHandlerBinanceOrderAllowCancelStopLoss : public APIHandlerBinanceCancelReplaceOrder
{
public:
    APIHandlerBinanceOrderAllowCancelStopLoss(HttpRequest* request);

    static Json handle_internal_request(const std::string& user_id, Json& query_json, Json& price_ticker);

protected:
    virtual std::string get_query(Json& query_json);
};

#endif //API_HANDLER_BINANCE_ORDER_ALLOW_CANCEL_STOP_LOSS_H