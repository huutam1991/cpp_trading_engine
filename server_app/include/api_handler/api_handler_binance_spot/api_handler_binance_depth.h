#ifndef API_HANDLER_BINANCE_DEPTH_H
#define API_HANDLER_BINANCE_DEPTH_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

class APIHandlerBinanceDepth : public APIHandlerBinance
{
public:
    APIHandlerBinanceDepth(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

    Json get_depth_from_MongoDB_by_symbol_stream(const std::string& symbol, const std::string& limit);
    Json get_depth_from_Binance(const std::string& symbol, const std::string& limit);
};

#endif //API_HANDLER_BINANCE_DEPTH_H