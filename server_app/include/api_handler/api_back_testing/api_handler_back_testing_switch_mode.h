#ifndef API_HANDLER_BACK_TESTING_SWITCH_MODE_H
#define API_HANDLER_BACK_TESTING_SWITCH_MODE_H

#include <api_handler/api_handler.h>

class APIHandlerBackTestingSwitchMode : public APIHandler
{
public:
    APIHandlerBackTestingSwitchMode(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_BACK_TESTING_SWITCH_MODE_H