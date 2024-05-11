#include <api_handler/api_back_testing/api_handler_back_testing_set_config.h>
#include <back_testing/back_testing.h>

APIHandlerBackTestingSetConfig::APIHandlerBackTestingSetConfig(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"speed_time", "db_name", "is_start"});
}

HttpResponse APIHandlerBackTestingSetConfig::child_handle()
{
    Json body = m_request->get_body_json();

    double speed_time = body["speed_time"];
    std::string db_name = body["db_name"];
    bool is_start = body["is_start"];

    // Handle config
    BackTesting::instance().set_speed_time(speed_time);
    BackTesting::instance().set_db_name(db_name);
    BackTesting::instance().set_start(is_start);

    // Response to client
    Json response;
    response["data"] = m_request->get_body_json();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(ResponseStatusCode::OK_200, response);
}