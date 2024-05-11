
#ifndef API_HANDLER_BINANCE_FUTURES_EX_INFO_H
#define API_HANDLER_BINANCE_FUTURES_EX_INFO_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

/*  Current exchange trading rules and symbol information
    https://binance-docs.github.io/apidocs/futures/en/#exchange-information
*/
class APIHandlerBinanceFuturesExchangeInfo : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFuturesExchangeInfo(HttpRequest* request = nullptr);

private:
    virtual HttpResponse child_handle();

    Json get_exchange_info();
};

#endif //API_HANDLER_BINANCE_FUTURES_EX_INFO_H