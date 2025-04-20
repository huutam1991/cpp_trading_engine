#ifndef API_HANDLER_STRATEGY_PA_CONFIG_H
#define API_HANDLER_STRATEGY_PA_CONFIG_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyPAConfig : public APIHandler
{
public:
    APIHandlerStrategyPAConfig(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_PA_CONFIG_H