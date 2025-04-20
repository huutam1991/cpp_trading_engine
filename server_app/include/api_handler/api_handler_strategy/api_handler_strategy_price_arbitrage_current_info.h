#ifndef API_HANDLER_STRATEGY_PA_CURRENT_INFO_H
#define API_HANDLER_STRATEGY_PA_CURRENT_INFO_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyPACurrentInfo : public APIHandler
{
public:
    APIHandlerStrategyPACurrentInfo(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_PA_CURRENT_INFO_H