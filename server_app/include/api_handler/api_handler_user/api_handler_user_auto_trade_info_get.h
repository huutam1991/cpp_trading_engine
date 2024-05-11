#ifndef API_HANDLER_USER_AUTO_TRADE_INFO_GET_H
#define API_HANDLER_USER_AUTO_TRADE_INFO_GET_H

#include <api_handler/api_handler.h>

class APIHandlerUserAutoTradeInfoGet : public APIHandler
{
public:
    APIHandlerUserAutoTradeInfoGet(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_AUTO_TRADE_INFO_GET_H