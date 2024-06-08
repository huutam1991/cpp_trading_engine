#include <api_handler/api_handler_strategy/api_handler_strategy_current_info.h>
#include <strategy/strategy.h>

APIHandlerStrategyCurrentInfo::APIHandlerStrategyCurrentInfo(HttpRequest* request) : APIHandler(request)
{
}

HttpResponse APIHandlerStrategyCurrentInfo::child_handle()
{
    Json response;

    // Response
    response["data"] = Strategy::instance().get_current_info();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);;
}