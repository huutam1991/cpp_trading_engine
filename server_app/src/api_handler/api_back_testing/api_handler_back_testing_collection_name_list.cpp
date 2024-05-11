#include <api_handler/api_back_testing/api_handler_back_testing_collection_name_list.h>
#include <back_testing/back_testing.h>

APIHandlerBackTestingCollectionNameList::APIHandlerBackTestingCollectionNameList(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_params({"db_name"});
}

HttpResponse APIHandlerBackTestingCollectionNameList::child_handle()
{
    std::string db_name = m_request->get_query_param("db_name");

    // Response to client
    Json response;
    response["data"] = BackTesting::instance().get_collection_name_list_by_db_name(db_name);
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(ResponseStatusCode::OK_200, response);
}