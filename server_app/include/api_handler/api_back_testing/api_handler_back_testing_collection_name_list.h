#ifndef API_HANDLER_BACK_TESTING_COLLECTION_NAME_LIST_H
#define API_HANDLER_BACK_TESTING_COLLECTION_NAME_LIST_H

#include <api_handler/api_handler.h>

class APIHandlerBackTestingCollectionNameList : public APIHandler
{
public:
    APIHandlerBackTestingCollectionNameList(HttpRequest* request);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_BACK_TESTING_COLLECTION_NAME_LIST_H