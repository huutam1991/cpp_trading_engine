#ifndef API_HANDLER_COINBASE_GET_ALL_FILLS_H
#define API_HANDLER_COINBASE_GET_ALL_FILLS_H

#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>

class APIHandlerCoinbaseGetAllFills : public APIHandlerCoinbase
{
public:
    APIHandlerCoinbaseGetAllFills(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_COINBASE_GET_ALL_FILLS_H