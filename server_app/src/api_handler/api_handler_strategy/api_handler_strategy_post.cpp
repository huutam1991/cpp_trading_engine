#include <api_handler/api_handler_strategy/api_handler_strategy_post.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerStrategyPost::APIHandlerStrategyPost(HttpRequest* request) : APIHandler(request)
{
    // m_need_check_authentication = true;
}

HttpResponse APIHandlerStrategyPost::child_handle()
{
    Json strategy_inputs = m_request->get_body_json();

    ADD_LOG("APIHandlerStrategyPost: " << strategy_inputs);
    
    ResponseStatusCode status_code = 
        ScanningStrategiesManager::instance().start_strategy(strategy_inputs);

    Json response;
    response["data"] = "";
    response["status_code"] = status_code;
    if (status_code == OK_200)
    {
        response["msg"] = "Start strategy successfully";
        response["error"] = false;
    }
    else if (status_code == CREATED_201)
    {
        response["msg"] = "Strategy had already existed";
        response["error"] = true;
    }
    else if (status_code == BAD_REQUEST_400)
    {
        response["msg"] = "Your input(s) is incorrect";
        response["error"] = true;
    }

    return HttpResponse(status_code, response);
}