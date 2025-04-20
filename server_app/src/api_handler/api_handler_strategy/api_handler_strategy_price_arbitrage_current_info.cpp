#include <api_handler/api_handler_strategy/api_handler_strategy_price_arbitrage_current_info.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>

APIHandlerStrategyPACurrentInfo::APIHandlerStrategyPACurrentInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

HttpResponse APIHandlerStrategyPACurrentInfo::child_handle()
{
    Json response;

    // Response
    response["data"] = StrategyPriceArbitrage::instance().get_current_info();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);;
}