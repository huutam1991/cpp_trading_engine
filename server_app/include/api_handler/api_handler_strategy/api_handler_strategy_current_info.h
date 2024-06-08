#ifndef API_HANDLER_STRATEGY_CURRENT_INFO_H
#define API_HANDLER_STRATEGY_CURRENT_INFO_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyCurrentInfo : public APIHandler
{
public:
    APIHandlerStrategyCurrentInfo(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_CURRENT_INFO_H