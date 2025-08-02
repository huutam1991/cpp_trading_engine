#include <api_handler/api_handler_account/api_handler_add_account.h>
#include <account/account.h>

APIHandlerAddAccount::APIHandlerAddAccount(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"key"});
}

Task<HttpResponse> APIHandlerAddAccount::child_handle()
{
    Json response;
    Json account = m_request->get_body_json();

    std::string key = account["key"];
    Json find_account = Account::load_account_by_key(key);

    if (find_account != nullptr)
    {
        response["data"] = "";
        response["msg"] = "account [" + key + "] already exist";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        Account::save_account_to_db(account);

        // Response
        response["data"] = {};
        response["msg"] = "register account [" + key + "] successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    co_return HttpResponse(OK_200, response);
}