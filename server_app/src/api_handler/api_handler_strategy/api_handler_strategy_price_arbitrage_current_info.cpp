#include <api_handler/api_handler_strategy/api_handler_strategy_price_arbitrage_current_info.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <strategy_mean_reversion/strategy_mean_reversion.h>

APIHandlerStrategyPACurrentInfo::APIHandlerStrategyPACurrentInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"type"});
}

HttpResponse APIHandlerStrategyPACurrentInfo::child_handle()
{
    Json data;

    // Check request parameter
    std::string type = m_request->get_query_param("type");
    if (type == "orders_chain")
    {
        data = StrategyPriceArbitrage::instance().get_orders_chain();
        // data = StrategyMeanReversion::instance().get_orders_chain();
    }
    else if (type == "open_orders")
    {
        data = StrategyPriceArbitrage::instance().get_open_orders();
        // data = StrategyMeanReversion::instance().get_open_orders();
    }

    // Response
    Json response;
    response["data"] = data;
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);;
}