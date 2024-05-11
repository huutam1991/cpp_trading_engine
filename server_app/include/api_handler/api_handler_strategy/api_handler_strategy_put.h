#ifndef API_HANDLER_STRATEGY_PUT_H
#define API_HANDLER_STRATEGY_PUT_H

#include <api_handler/api_handler.h>

class APIHandlerStrategyPut : public APIHandler
{
public:
    APIHandlerStrategyPut(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
    ResponseStatusCode update_strategy(const long _id, 
                                            Json& strategy_inputs,
                                            Json& response);
    ResponseStatusCode stop_strategy(const long _id, 
                                            Json& response);
    ResponseStatusCode stop_all_strateies(Json& response);

};

#endif //API_HANDLER_STRATEGY_PUT_H