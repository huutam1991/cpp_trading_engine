#include <api_handler/api_handler_user/api_handler_user_auto_trade_info_get.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerUserAutoTradeInfoGet::APIHandlerUserAutoTradeInfoGet(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

HttpResponse APIHandlerUserAutoTradeInfoGet::child_handle()
{
    ADD_LOG("APIHandlerUserAutoTradeInfoGet");
    Json response;
    response["data"] = ScanningStrategiesManager::instance().get_all_auto_trading_info();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}
