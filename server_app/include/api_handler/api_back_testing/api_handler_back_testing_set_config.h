#ifndef API_HANDLER_BACK_TESTING_SET_CONFIG_H
#define API_HANDLER_BACK_TESTING_SET_CONFIG_H

#include <api_handler/api_handler.h>

class APIHandlerBackTestingSetConfig : public APIHandler
{
public:
    APIHandlerBackTestingSetConfig(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_BACK_TESTING_SET_CONFIG_H