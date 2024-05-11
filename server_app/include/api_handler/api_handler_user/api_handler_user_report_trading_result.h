#ifndef API_HANDLER_USER_REPORT_TRADIING_RESULT_H
#define API_HANDLER_USER_REPORT_TRADIING_RESULT_H

#include <api_handler/api_handler.h>

class APIHandlerUserReportTradingResult : public APIHandler
{
public:
    APIHandlerUserReportTradingResult(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_REPORT_TRADIING_RESULT_H