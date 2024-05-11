#include <api_handler/api_handler_user/api_handler_user_config.h>
#include <app_constants.h>
#include <utils.h>

APIHandlerUserConfig::APIHandlerUserConfig(HttpRequest* request) : APIHandler(request)
{}

HttpResponse APIHandlerUserConfig::child_handle()
{
    Json user_config = Json::parse(m_request->get_body());
    std::string symbol = user_config["symbol"]["value"];
    std::string source = user_config["source"]["value"];

    // Check missing symbol's valur or source's value
    if (symbol == "")
    {
        return HttpRequest::response_bad_request_400("Missing symbol's value");
    }
    if (source == "")
    {
        return HttpRequest::response_bad_request_400("Missing source's value");
    }

    user_config["sym"] = symbol;
    user_config["src"] = source;

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(BINANCE_COMMON, "user_config");

    size_t symbol_count = query.count_documents("sym", symbol);
    if (symbol_count == 0)
    {
        query.insert_one(user_config);
    }
    else
    {
        query.replace_one("sym", symbol, user_config);
    }

    Json response;
    response["data"] = "";
    response["msg"] = "Update user config successully";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}
