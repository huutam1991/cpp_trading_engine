#ifndef API_HANDLER_BINANCE_ALL_SYMBOLS_H
#define API_HANDLER_BINANCE_ALL_SYMBOLS_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

class APIHandlerBinanceAllSymbols : public APIHandlerBinance
{
public:
    APIHandlerBinanceAllSymbols(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_BINANCE_ALL_SYMBOLS_H