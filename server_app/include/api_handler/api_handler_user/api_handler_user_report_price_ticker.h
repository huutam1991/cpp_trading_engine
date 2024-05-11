#ifndef API_HANDLER_USER_REPORT_PRICE_TICKER_H
#define API_HANDLER_USER_REPORT_PRICE_TICKER_H

#include <api_handler/api_handler.h>

class APIHandlerUserReportPriceTicker : public APIHandler
{
public:
    APIHandlerUserReportPriceTicker(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_REPORT_PRICE_TICKER_H