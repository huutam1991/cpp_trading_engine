#include <api_handler/api_handler_strategy/api_handler_strategy_config.h>
#include <strategy/strategy_manager.h>

#include <c_json/json.h>

APIHandlerStrategyConfig::APIHandlerStrategyConfig(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"strategy_name"});
}

Task<HttpResponse> APIHandlerStrategyConfig::child_handle()
{
    JsonNew response;
    std::string strategy_name = m_request->get_query_param("strategy_name");

    // GET
    if (m_request->get_request_method() == RequestMethod::GET)
    {
        JsonNew config = StrategyManager::instance().get_config_by_strategy(strategy_name);

        if (config.has_field("code") == false)
        {
            response["data"] = config;
            response["msg"] = "";
            response["status_code"] = OK_200;
            response["error"] = false;
        }
        else
        {
            response["data"] = {};
            response["msg"] = config;
            response["status_code"] = OK_200;
            response["error"] = true;
        }

    }
    // POST
    else
    {
        JsonNew config_data = m_request->get_body_json();
        JsonNew config = JsonNew::parse(config_data.get_string_value());
        JsonNew update_result = StrategyManager::instance().update_config_by_strategy(strategy_name, config);

        if (update_result.has_field("code") && (int)update_result["code"] < 0)
        {
            response["data"] = config;
            response["msg"] = update_result;
            response["status_code"] = OK_200;
            response["error"] = true;
        }
        else
        {
            response["data"] = config;
            response["msg"] = "update config for strategy [" + strategy_name + "] successfully";
            response["status_code"] = OK_200;
            response["error"] = false;
        }
    }

    co_return HttpResponse(OK_200, response);;
}