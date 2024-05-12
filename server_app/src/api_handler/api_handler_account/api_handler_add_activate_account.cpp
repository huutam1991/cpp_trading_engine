#include <api_handler/api_handler_account/api_handler_add_activate_account.h>
#include <account/account.h>

APIHandlerAddActivateAccount::APIHandlerAddActivateAccount(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"key"});
}

HttpResponse APIHandlerAddActivateAccount::child_handle()
{
    Json response;
    Json account = m_request->get_body_json();
    std::string key = account["key"];

    Json find_account = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "activate_accounts")
        .find_one("key", key);

    if (find_account.is_null() == false)
    {
        response["data"] = "";
        response["msg"] = "activate account [" + key + "] already exist";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        MongoDB::instance()
            .set_db_and_collection(APP_INFO_DB_NAME, "activate_accounts")
            .insert_one(account);

        // Response
        response["data"] = {};
        response["msg"] = "register account [" + key + "] successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return HttpResponse(OK_200, response);
}