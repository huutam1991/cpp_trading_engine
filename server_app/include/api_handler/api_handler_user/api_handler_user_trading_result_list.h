#ifndef API_HANDLER_USER_TRADIING_RESULT_LIST_H
#define API_HANDLER_USER_TRADIING_RESULT_LIST_H

#include <api_handler/api_handler.h>

class APIHandlerUserTradingResultList : public APIHandler
{
public:
    APIHandlerUserTradingResultList(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

    Json formalize_trading_strategy_result(Json& trading_strategy_result);
};

#endif //API_HANDLER_USER_TRADIING_RESULT_LIST_H