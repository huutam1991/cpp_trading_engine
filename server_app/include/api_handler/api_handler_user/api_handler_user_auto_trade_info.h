#ifndef API_HANDLER_USER_AUTO_TRADE_INFO_H
#define API_HANDLER_USER_AUTO_TRADE_INFO_H

#include <api_handler/api_handler.h>

class APIHandlerUserAutoTradeInfo : public APIHandler
{
public:
    APIHandlerUserAutoTradeInfo(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_AUTO_TRADE_INFO_H