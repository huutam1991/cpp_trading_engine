#include <api_handler/api_handler_strategy/api_handler_strategy_put.h>
#include <strategy_engine/scanning_strategies_manager.h>

APIHandlerStrategyPut::APIHandlerStrategyPut(HttpRequest* request) : APIHandler(request)
{
    // add_mandatory_params({"strategy_inputs"});
    // m_need_check_authentication = true;
}

HttpResponse APIHandlerStrategyPut::child_handle()
{
    Json response;
    response["data"] = "";

    Json strategy_inputs = m_request->get_body_json();
    ADD_LOG("APIHandlerStrategyPut: " << strategy_inputs);

    long _id = static_cast<long>(strategy_inputs["id"]);
    string request_type = (string&&)strategy_inputs["request_type"];

    ResponseStatusCode status_code;
    if (request_type == "stop")
    {
        if (_id == 0)
            status_code = stop_all_strateies(response);
        else
            status_code = stop_strategy(_id, response);
    }
    else
    {
        status_code = update_strategy(_id, strategy_inputs, response);
    }   

    return HttpResponse(status_code, response);
}

ResponseStatusCode APIHandlerStrategyPut::update_strategy(const long _id, 
                                            Json& strategy_inputs,
                                            Json& response)
{
    ResponseStatusCode status_code = ScanningStrategiesManager::instance().update_strategy(_id, strategy_inputs);

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
        else 
            response["msg"] = "Unknow error with id[" + to_string(_id) + "]";
    }

    return status_code;
}

ResponseStatusCode APIHandlerStrategyPut::stop_strategy(const long _id, 
                                            Json& response)
{
    ResponseStatusCode status_code = ScanningStrategiesManager::instance().stop_strategy(_id);

    response["status_code"] = status_code;

    if (status_code == OK_200)
    {
        response["msg"] = "Stop strategy with id[" + to_string(_id) + "] successfully";
        response["error"] = false;
    }
    else
    {
        response["error"] = true;
        if (status_code == BAD_REQUEST_400)
            response["msg"] = "Can't stop strategy with id[" + to_string(_id) + "]";
        else if (status_code == NOT_FOUND_404)
            response["msg"] = "Can't find id[" + to_string(_id) + "] on the system";
        else 
            response["msg"] = "Unknow error with id[" + to_string(_id) + "]";
    }

    return status_code;
}

ResponseStatusCode APIHandlerStrategyPut::stop_all_strateies(Json& response)
{
    ScanningStrategiesManager::instance().stop_all_strategies();
    return OK_200;
}