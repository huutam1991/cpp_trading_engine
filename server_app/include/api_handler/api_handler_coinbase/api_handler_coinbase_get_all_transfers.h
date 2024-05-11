#ifndef API_HANDLER_COINBASE_GET_ALL_TRANSFERS_H
#define API_HANDLER_COINBASE_GET_ALL_TRANSFERS_H

#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>

class APIHandlerCoinbaseGetAllTransfers : public APIHandlerCoinbase
{
public:
    APIHandlerCoinbaseGetAllTransfers(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_COINBASE_GET_ALL_TRANSFERS_H