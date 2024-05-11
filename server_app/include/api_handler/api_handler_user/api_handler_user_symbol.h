#ifndef API_HANDLER_USER_SYMBOL_H
#define API_HANDLER_USER_SYMBOL_H

#include <api_handler/api_handler.h>

class APIHandlerUserSymbol : public APIHandler
{
public:
    APIHandlerUserSymbol(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_SYMBOL_H