#ifndef API_HANDLER_USER_UPDATE_SOURCE_INFO_H
#define API_HANDLER_USER_UPDATE_SOURCE_INFO_H

#include <api_handler/api_handler.h>

class APIHandlerUserUpdateSourceInfo : public APIHandler
{
public:
    APIHandlerUserUpdateSourceInfo(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_UPDATE_SOURCE_INFO_H