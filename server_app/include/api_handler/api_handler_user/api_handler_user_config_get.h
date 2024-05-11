#ifndef API_HANDLER_USER_CONFIG_GET_H
#define API_HANDLER_USER_CONFIG_GET_H

#include <api_handler/api_handler.h>

class APIHandlerUserConfigGet : public APIHandler
{
public:
    APIHandlerUserConfigGet(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_CONFIG_GET_H