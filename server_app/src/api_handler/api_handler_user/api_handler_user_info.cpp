#include <api_handler/api_handler_user/api_handler_user_info.h>
#include <app_utils.h>

APIHandlerUserInfo::APIHandlerUserInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

HttpResponse APIHandlerUserInfo::child_handle()
{
    Json response;
    User* user = m_user.get();

    response["data"] = {
        {"balance",            AppUtils::instance().get_balance_by_user(user)},
        {"symbols",            AppUtils::instance().get_symbols_in_use_by_user(user)},
        {"registered_sources", AppUtils::instance().get_register_source_by_user(user)},
        {"active_source",      m_user->get_active_storage_source()->get_source_info()},
        {"user_id",            m_user->get_user_id()},
    };
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}