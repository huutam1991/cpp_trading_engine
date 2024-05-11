#ifndef API_HANDLER_USER_PRICE_TICKER_LIST_H
#define API_HANDLER_USER_PRICE_TICKER_LIST_H

#include <api_handler/api_handler.h>

class APIHandlerUserPriceTickerList : public APIHandler
{
public:
    APIHandlerUserPriceTickerList(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

    Json get_price_ticker_list(long from, long to);
};

#endif //API_HANDLER_USER_PRICE_TICKER_LIST_H