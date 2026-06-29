#include <api_handler/api_handler_account/api_handler_add_account.h>
#include <account/account_manager.h>

APIHandlerAddAccount::APIHandlerAddAccount(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"key"});
}

Task<HttpResponse> APIHandlerAddAccount::child_handle()
{
    Json response;
    Json account = m_request->get_body_json();

    auto result = AccountManager::add_account(account);

    if (result.has_value() == false)
    {
        response["data"] = "";
        response["msg"] = result.error();
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        // Response
        response["data"] = {};
        response["msg"] = "Register account [" + (std::string)account["key"] + "] successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    co_return HttpResponse(OK_200, response);
}