#include <api_handler/api_handler_strategy/api_handler_strategy_price_arbitrage_config.h>
#include <mongo_db/mongo_db.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <strategy_mean_reversion/strategy_mean_reversion.h>

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

    // add_mandatory_body_params({
    //     "symbol",
    //     "buy_volumn",
    //     "buy_at_lower_price",
    //     "sell_at_higher_price",
    //     "too_low_price_delta",
    //     "too_high_price_delta",
    //     "is_running"
    // });
}

Task<HttpResponse> APIHandlerStrategyPAConfig::child_handle()
{
    Json response;
    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "price_arbitrage_config");
    // MongoQuery query = MongoDB::instance()
    //     .set_db_and_collection(STRATEGY_DB_NAME, "mean_reversion_config");
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
        std::string symbol = config["symbol"];

        if (current_config == nullptr)
        {
            query.insert_one(config);
        }
        else
        {
            std::string _id = current_config["_id"]["$oid"];
            query.replace_one("_id", bsoncxx::oid(_id), config);
        }

        // Re-init Strategy with new config
        // StrategyPriceArbitrage::instance().on_config_change();
        // StrategyMeanReversion::instance().on_config_change();

        // Response
        response["data"] = config;
        response["msg"] = "update config for strategy [price arbitrage] [" + symbol + "] successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    co_return HttpResponse(OK_200, response);;
}