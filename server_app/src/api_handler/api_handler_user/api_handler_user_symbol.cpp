#include <api_handler/api_handler_user/api_handler_user_symbol.h>

APIHandlerUserSymbol::APIHandlerUserSymbol(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"symbols"});
}

HttpResponse APIHandlerUserSymbol::child_handle()
{
    Json symbols = m_request->get_body_param_json("symbols");
    std::string user_id = m_user->get_user_id();

    // Data
    Json data = {
        {"user_id", user_id},
        {"symbols", symbols}
    };

    // Query
    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(USER_DB_NAME, "symbols_in_use");

    int count = query.find_one("user_id", user_id);
    if (count == 0)
    {
        query.insert_one(data);
    }
    else
    {
        query.replace_one("user_id", user_id, data);
    }

    // Response
    Json response;
    response["data"] = symbols;
    response["msg"] = "Updated symbols in use for user [" + user_id + "] successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}
