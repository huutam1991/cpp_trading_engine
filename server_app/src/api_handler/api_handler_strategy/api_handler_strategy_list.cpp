#include <api_handler/api_handler_strategy/api_handler_strategy_list.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerStrategyList::APIHandlerStrategyList(HttpRequest* request) : APIHandler(request)
{
    // m_need_check_authentication = true;
}

HttpResponse APIHandlerStrategyList::child_handle()
{
    Json response;
    response["data"] = ScanningStrategiesManager::instance().get_all_auto_trading_info();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    ADD_LOG("APIHandlerStrategyList response: " << response.get_string_value());

    return HttpResponse(OK_200, response);
}