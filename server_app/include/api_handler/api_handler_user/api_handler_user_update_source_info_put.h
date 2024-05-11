#ifndef API_HANDLER_USER_UPDATE_SOURCE_INFO_PUT_H
#define API_HANDLER_USER_UPDATE_SOURCE_INFO_PUT_H

#include <api_handler/api_handler.h>

class APIHandlerUserUpdateSourceInfoPut : public APIHandler
{
public:
    APIHandlerUserUpdateSourceInfoPut(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_UPDATE_SOURCE_INFO_PUT_H