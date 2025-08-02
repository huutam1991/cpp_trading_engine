#include <api_handler/api_handler_strategy/api_handler_strategy_current_info.h>
#include <strategy/strategy_manager.h>

APIHandlerStrategyCurrentInfo::APIHandlerStrategyCurrentInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"strategy_name"});
}

Task<HttpResponse> APIHandlerStrategyCurrentInfo::child_handle()
{
    Json response;
    Json params = m_request->get_body_json();
    std::string strategy_name = m_request->get_query_param("strategy_name");

    // Response
    response["data"] = StrategyManager::instance().get_info(strategy_name, params);
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}