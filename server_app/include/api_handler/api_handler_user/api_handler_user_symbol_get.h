#ifndef API_HANDLER_USER_SYMBOL_GET_H
#define API_HANDLER_USER_SYMBOL_GET_H

#include <api_handler/api_handler.h>

class APIHandlerUserSymbolGet : public APIHandler
{
public:
    APIHandlerUserSymbolGet(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_SYMBOL_GET_H