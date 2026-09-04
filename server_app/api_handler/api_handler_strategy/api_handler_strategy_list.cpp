#include <api_handler/api_handler_strategy/api_handler_strategy_list.h>
#include <strategy/strategy_manager.h>

APIHandlerStrategyList::APIHandlerStrategyList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"strategy_name"});
}

Task<HttpResponse> APIHandlerStrategyList::child_handle()
{
    co_return HttpResponse
    (
        OK_200,
        {
            {"data", StrategyManager::instance().get_strategy_list()},
            {"msg", "Get strategy list successfully"},
            {"status_code", OK_200},
            {"error", false}
        }
    );
}