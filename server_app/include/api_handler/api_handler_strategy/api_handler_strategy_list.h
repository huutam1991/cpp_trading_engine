#ifndef API_HANDLER_STRATEGY_LIST_H
#define API_HANDLER_STRATEGY_LIST_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyList : public APIHandler
{
public:
    APIHandlerStrategyList(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_LIST_H