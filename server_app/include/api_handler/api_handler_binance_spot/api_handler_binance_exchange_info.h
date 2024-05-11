#ifndef API_HANDLER_BINANCE_EXCHANGE_INFO_H
#define API_HANDLER_BINANCE_EXCHANGE_INFO_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

/*  Current exchange trading rules and symbol information
    https://binance-docs.github.io/apidocs/spot/en/#exchange-information
*/
class APIHandlerBinanceExchangeInfo : public APIHandlerBinance
{
public:
    APIHandlerBinanceExchangeInfo(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

    Json get_symbol_info();
};

#endif //API_HANDLER_BINANCE_EXCHANGE_INFO_H