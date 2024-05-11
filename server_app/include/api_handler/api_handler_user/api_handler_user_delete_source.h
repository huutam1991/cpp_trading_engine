#ifndef API_HANDLER_USER_DELETE_SOURCE_H
#define API_HANDLER_USER_DELETE_SOURCE_H

#include <api_handler/api_handler.h>

class APIHandlerUserDeleteSource : public APIHandler
{
public:
    APIHandlerUserDeleteSource(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_DELETE_SOURCE_H