#include <api_handler/api_handler_account/api_handler_set_active_account.h>
#include <account/account_manager.h>

APIHandlerSetActiveAccount::APIHandlerSetActiveAccount(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"key", "is_active"});
}

Task<HttpResponse> APIHandlerSetActiveAccount::child_handle()
{
    Json account = m_request->get_body_json();
    std::string key = account["key"];
    bool is_active = account["is_active"];

    std::expected<bool, std::string> result = AccountManager::set_active_account(key, is_active);
    if (result.has_value() == false)
    {
        Json response;
        response["msg"] = result.error();
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        co_return HttpResponse(BAD_REQUEST_400, response);
    }

    Json response;
    response["msg"] = "Account updated successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}