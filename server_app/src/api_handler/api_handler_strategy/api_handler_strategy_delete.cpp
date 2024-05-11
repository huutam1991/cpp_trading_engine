#include <api_handler/api_handler_strategy/api_handler_strategy_delete.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerStrategyDelete::APIHandlerStrategyDelete(HttpRequest* request) : APIHandler(request)
{
    // m_need_check_authentication = true;
}

HttpResponse APIHandlerStrategyDelete::child_handle()
{
    long _id = stol(m_request->get_body_param_string("strategy_id"));
    ADD_LOG("APIHandlerStrategyDelete: " << _id);

    ResponseStatusCode status_code = 
        ScanningStrategiesManager::instance().remove_strategy(_id);

    Json response;
    response["data"] = "";
    response["status_code"] = status_code;
    if (status_code == OK_200)
    {
        response["msg"] = "Stop strategy id[" + m_request->get_query_param("id") + "] successfully";
        response["error"] = false;
    }
    else if (status_code == NOT_FOUND_404)
    {
        response["msg"] = "Strategy id[" + m_request->get_query_param("id") + "] doesn't exist";
        response["error"] = true;
    }

    return HttpResponse(OK_200, response);
}