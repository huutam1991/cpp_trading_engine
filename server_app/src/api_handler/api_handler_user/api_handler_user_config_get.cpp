#include <api_handler/api_handler_user/api_handler_user_config_get.h>
#include <utils.h>
#include <app_constants.h>

APIHandlerUserConfigGet::APIHandlerUserConfigGet(HttpRequest* request) : APIHandler(request)
{}

HttpResponse APIHandlerUserConfigGet::child_handle()
{
    Json response;
    const std::string symbol = m_request->get_query_param("symbol");

    Json user_config = MongoDB::instance()
        .set_db_and_collection(BINANCE_COMMON, "user_config")
        .find_one("sym", symbol);

    if (user_config.is_null() == false)
    {
        user_config.remove_field("_id");
        user_config.remove_field("sym");
        user_config.remove_field("src");

        response["data"] = user_config;
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    }
    else
    {
        response["data"] = "";
        response["msg"] = "Cannot find user's config with symbol is " + symbol;
        response["status_code"] = INTERNAL_SERVER_ERROR_500;
        response["error"] = true;

        return HttpResponse(OK_200, response);
    }
}
