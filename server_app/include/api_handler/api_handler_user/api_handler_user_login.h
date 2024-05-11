#ifndef API_HANDLER_USER_LOGIN_H
#define API_HANDLER_USER_LOGIN_H

#include <api_handler/api_handler.h>

class APIHandlerUserLogin : public APIHandler
{
public:
    APIHandlerUserLogin(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_LOGIN_H