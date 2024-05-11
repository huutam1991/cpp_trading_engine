#include <api_handler/api_handler_user/api_handler_user_auto_trade_info.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerUserAutoTradeInfo::APIHandlerUserAutoTradeInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"strategy_inputs"});
}

HttpResponse APIHandlerUserAutoTradeInfo::child_handle()
{
    Json strategy_inputs = m_request->get_body_param_json("strategy_inputs");
    long _id = (long)(strategy_inputs["id"]);

    ResponseStatusCode status_code = 
        ScanningStrategiesManager::instance().update_strategy(_id, strategy_inputs);

    Json response;
    response["data"] = "";
    response["status_code"] = status_code;

    if (status_code == OK_200)
    {
        response["msg"] = "Updated auto trade info for id[" + to_string(_id) + "] successfully";
        response["error"] = false;
    }
    else
    {
        response["error"] = true;
        if (status_code == BAD_REQUEST_400)
            response["msg"] = "Can't change asset or scan_symbol of id[" + to_string(_id) + "]";
        else if (status_code == NOT_FOUND_404)
            response["msg"] = "Can't find id[" + to_string(_id) + "] on the system";
        else if (status_code == INTERNAL_SERVER_ERROR_500)
            response["msg"] = "The hedging strategy of id[" + to_string(_id) + "] is running";
    }

    return HttpResponse(status_code, response);
}
