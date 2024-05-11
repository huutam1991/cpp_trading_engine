#ifndef API_HANDLER_USER_CONFIG_H
#define API_HANDLER_USER_CONFIG_H

#include <api_handler/api_handler.h>

class APIHandlerUserConfig : public APIHandler
{
public:
    APIHandlerUserConfig(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_CONFIG_H