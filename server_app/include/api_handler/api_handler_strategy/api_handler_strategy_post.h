#ifndef API_HANDLER_STRATEGY_POST_H
#define API_HANDLER_STRATEGY_POST_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyPost : public APIHandler
{
public:
    APIHandlerStrategyPost(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_POST_H