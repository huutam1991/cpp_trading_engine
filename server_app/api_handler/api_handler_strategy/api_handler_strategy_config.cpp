#include <api_handler/api_handler_strategy/api_handler_strategy_config.h>
#include <strategy/strategy_manager.h>

APIHandlerStrategyConfig::APIHandlerStrategyConfig(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"strategy_name"});
}

Task<HttpResponse> APIHandlerStrategyConfig::child_handle()
{
    Json response;

    // GET
    if (m_request->get_request_method() == RequestMethod::GET)
    {
        std::string strategy_name = m_request->get_query_param("strategy_name");
        Json config = StrategyManager::instance().get_config_by_strategy(strategy_name);

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
        // Json config = m_request->get_body_json();
        // std::string symbol = config["symbol"];
        // double buy_volumn = config["buy_volumn"];
        // double move_value = config["move_price"];

        // if (current_config.is_null() == true)
        // {
        //     query.insert_one(config);
        // }
        // else
        // {
        //     std::string _id = current_config["_id"]["$oid"];
        //     query.replace_one("_id", bsoncxx::oid(_id), config);
        // }

        // // Re-init Strategy with new config
        // Strategy::instance().on_config_change();

        // // Response
        // response["data"] = config;
        // response["msg"] = "update config for strategy [" + symbol + "_" + std::to_string((size_t)buy_volumn) + "_" + std::to_string((size_t)move_value) + "] successfully";
        // response["status_code"] = OK_200;
        // response["error"] = false;
    }

    co_return HttpResponse(OK_200, response);;
}