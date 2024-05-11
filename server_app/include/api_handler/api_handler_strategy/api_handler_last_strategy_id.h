#ifndef API_HANDLER_LAST_STRATEGY_ID_H
#define API_HANDLER_LAST_STRATEGY_ID_H

#include <api_handler/api_handler.h>

class APIHandlerLastStrategyId : public APIHandler
{
public:
    APIHandlerLastStrategyId(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_LAST_STRATEGY_ID_H