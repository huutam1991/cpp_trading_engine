#ifndef API_HANDLER_USER_ACTIVE_SOURCE_H
#define API_HANDLER_USER_ACTIVE_SOURCE_H

#include <api_handler/api_handler.h>

class APIHandlerUserActiveSource : public APIHandler
{
public:
    APIHandlerUserActiveSource(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_USER_ACTIVE_SOURCE_H