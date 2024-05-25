#ifndef API_HANDLER_STRATEGY_CONFIG_H
#define API_HANDLER_STRATEGY_CONFIG_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyConfig : public APIHandler
{
public:
    APIHandlerStrategyConfig(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_CONFIG_H