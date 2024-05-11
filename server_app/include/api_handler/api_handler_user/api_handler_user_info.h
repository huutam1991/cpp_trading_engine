#ifndef API_HANDLER_USER_INFO_H
#define API_HANDLER_USER_INFO_H

#include <api_handler/api_handler.h>

class APIHandlerUserInfo : public APIHandler
{
public:
    APIHandlerUserInfo(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_INFO_H