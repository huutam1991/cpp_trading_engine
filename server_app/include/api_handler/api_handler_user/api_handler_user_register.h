#ifndef API_HANDLER_USER_REGISTER_H
#define API_HANDLER_USER_REGISTER_H

#include <api_handler/api_handler.h>

class APIHandlerUserRegister : public APIHandler
{
public:
    APIHandlerUserRegister(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_REGISTER_H