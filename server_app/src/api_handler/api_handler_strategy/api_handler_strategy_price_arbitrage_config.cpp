#include <api_handler/api_handler_strategy/api_handler_strategy_price_arbitrage_config.h>
#include <mongo_db/mongo_db.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>

APIHandlerStrategyPAConfig::APIHandlerStrategyPAConfig(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;

    add_mandatory_body_params({
        "symbol_1",
        "symbol_2",
        "symbol_3",
        "buy_volumn",
        "buy_at_lower_price",
        "price_delta",
        "too_low_price_delta",
        "too_high_price_delta",
        "is_running"
    });
}

HttpResponse APIHandlerStrategyPAConfig::child_handle()
{
    Json response;
    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "price_arbitrage_config");
    Json current_config = query.find_any();

    // GET
    if (m_request->get_request_method() == RequestMethod::GET)
    {
        current_config.remove_field("_id");

        // Response
        response["data"] = current_config;
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;
    }
    // POST
    else
    {
        Json config = m_request->get_body_json();
        std::string symbol_1 = config["symbol_1"];
        std::string symbol_2 = config["symbol_2"];
        std::string symbol_3 = config["symbol_3"];
        double buy_volumn = config["buy_volumn"];
        double buy_at_lower_price = config["buy_at_lower_price"];

        if (current_config.is_null() == true)
        {
            query.insert_one(config);
        }
        else
        {
            std::string _id = current_config["_id"]["$oid"];
            query.replace_one("_id", bsoncxx::oid(_id), config);
        }

        // Re-init Strategy with new config
        StrategyPriceArbitrage::instance().on_config_change();

        // Response
        response["data"] = config;
        response["msg"] = "update config for strategy [price arbitrage] [" + symbol_1 + "-" + symbol_2 + "-" + symbol_3 + "] successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return HttpResponse(OK_200, response);;
}