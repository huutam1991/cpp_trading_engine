#include <api_handler/api_handler_user/api_handler_user_trading_result_list.h>
#include <app_utils.h>

APIHandlerUserTradingResultList::APIHandlerUserTradingResultList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"from", "to"});
}

Json APIHandlerUserTradingResultList::formalize_trading_strategy_result(Json& trading_strategy_result)
{
    Json res = Json::create_array();

    trading_strategy_result.for_each([&res](Json& strategy)
    {
        strategy.for_each([&res](Json& symbol)
        {
            symbol.for_each([&res](Json& result)
            {
                result.remove_field("hit_time");
                result.remove_field("pair_name");
                Json symbol_list = result["symbol_list"];
                result["symbol_list"] = (std::string&&)symbol_list[0] + "-" + (std::string&&)symbol_list[1] + "-" + (std::string&&)symbol_list[2];
                res.push_back(result);
            });
        });
    });

    return res;
}

HttpResponse APIHandlerUserTradingResultList::child_handle()
{
    long from = std::stol(m_request->get_query_param("from"));
    long to   = std::stol(m_request->get_query_param("to"));
    Json trading_strategy_result = AppUtils::instance().get_trading_strategy_result(m_user.get(), from, to);

    Json response;
    response["data"] = formalize_trading_strategy_result(trading_strategy_result);
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}