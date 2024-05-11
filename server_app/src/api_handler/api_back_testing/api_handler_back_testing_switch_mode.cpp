#include <api_handler/api_back_testing/api_handler_back_testing_switch_mode.h>
#include <back_testing/back_testing.h>

APIHandlerBackTestingSwitchMode::APIHandlerBackTestingSwitchMode(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"back_testing_mode"});
}

HttpResponse APIHandlerBackTestingSwitchMode::child_handle()
{
    std::string back_testing_mode = m_request->get_body_param_string("back_testing_mode");
    bool is_back_testing_mode = false;

    ADD_LOG("APIHandlerBackTestingSwitchMode 1");

    if (back_testing_mode == "on")
    {
    ADD_LOG("APIHandlerBackTestingSwitchMode 2.1");
        is_back_testing_mode = BackTesting::instance().start_back_testing_mode();
    }
    else
    {
    ADD_LOG("APIHandlerBackTestingSwitchMode 2.2");
        is_back_testing_mode = BackTesting::instance().stop_back_testing_mode();
    }
    ADD_LOG("APIHandlerBackTestingSwitchMode 3");

    std::string status = is_back_testing_mode == true ? "on" : "off";
    Json back_testing_mode_result = {
        {"back_testing_mode", status}
    };

    // Response to client
    Json response;
    response["data"] = back_testing_mode_result;
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(ResponseStatusCode::OK_200, response);
}
