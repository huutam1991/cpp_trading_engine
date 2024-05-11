#include <app_utils.h>
#include <api_handler/api_handler_user/api_handler_user_symbol_get.h>

APIHandlerUserSymbolGet::APIHandlerUserSymbolGet(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

HttpResponse APIHandlerUserSymbolGet::child_handle()
{
    // Response
    Json response;
    response["data"] = AppUtils::instance().get_symbols_in_use_by_user(m_user.get());
    response["msg"] = "Symbols in use for user [" + m_user->get_user_id() + "]";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}
