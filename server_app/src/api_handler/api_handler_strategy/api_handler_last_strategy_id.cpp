#include <api_handler/api_handler_strategy/api_handler_last_strategy_id.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerLastStrategyId::APIHandlerLastStrategyId(HttpRequest* request) : APIHandler(request)
{
    // m_need_check_authentication = true;
}

HttpResponse APIHandlerLastStrategyId::child_handle()
{
    Json response;
    response["data"] = ScanningStrategiesManager::instance().get_last_strategy_id();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    ADD_LOG("APIHandlerLastStrategyId response: " << response.get_string_value());

    return HttpResponse(OK_200, response);
}