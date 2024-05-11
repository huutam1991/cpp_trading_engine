#ifndef API_HANDLER_COINBASE_CURRENT_PRICE_H
#define API_HANDLER_COINBASE_CURRENT_PRICE_H

#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>

class APIHandlerCoinbaseCurrentPrice : public APIHandlerCoinbase
{
public:
    APIHandlerCoinbaseCurrentPrice(HttpRequest* request);

    Json get_current_price_from_MongoDB_by_symbol_stream(const std::string& symbol);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_COINBASE_CURRENT_PRICE_H