#include <api_handler/api_handler_strategy/api_handler_strategy_config.h>
#include <mongo_db/mongo_db.h>

APIHandlerStrategyConfig::APIHandlerStrategyConfig(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"symbol", "volumn", "move_price", "is_running"});
}

HttpResponse APIHandlerStrategyConfig::child_handle()
{
    Json response;
    Json config = m_request->get_body_json();

    std::string symbol = config["symbol"];
    double price = config["volumn"];
    double move_value = config["move_price"];

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "config");

    Json current_config = query.find_any();

    if (current_config.is_null() == true)
    {
        query.insert_one(config);
    }
    else
    {
        std::string _id = current_config["_id"]["$oid"];
        query.replace_one("_id", bsoncxx::oid(_id), config);
    }

    // Response
    response["data"] = config;
    response["msg"] = "update config for strategy [" + symbol + "_" + std::to_string((size_t)price) + "_" + std::to_string((size_t)move_value) + "] successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);;
}