#ifndef API_HANDLER_SOURCE_AVAILABLE_H
#define API_HANDLER_SOURCE_AVAILABLE_H

#include <api_handler/api_handler.h>

class APIHandlerSourceAvailable : public APIHandler
{
public:
    APIHandlerSourceAvailable(HttpRequest* request);

private:
    virtual HttpResponse child_handle();
};

#endif //API_HANDLER_SOURCE_AVAILABLE_H