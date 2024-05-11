#ifndef API_HANDLER_STRATEGY_DELETE_H
#define API_HANDLER_STRATEGY_DELETE_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyDelete : public APIHandler
{
public:
    APIHandlerStrategyDelete(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_STRATEGY_DELETE_H