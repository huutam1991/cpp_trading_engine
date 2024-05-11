#ifndef API_HANDLER_USER_WEBSOCKET_TOKEN_H
#define API_HANDLER_USER_WEBSOCKET_TOKEN_H

#include <api_handler/api_handler.h>

class APIHandlerUserWebsocketToken : public APIHandler
{
public:
    APIHandlerUserWebsocketToken(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_WEBSOCKET_TOKEN_H