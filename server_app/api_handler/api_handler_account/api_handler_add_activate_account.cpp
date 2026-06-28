#include <api_handler/api_handler_account/api_handler_add_activate_account.h>
#include <account/account_db.h>

APIHandlerAddActivateAccount::APIHandlerAddActivateAccount(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"key", "exchange"});
}

Task<HttpResponse> APIHandlerAddActivateAccount::child_handle()
{
    Json response;
    Json account = m_request->get_body_json();
    std::string key = account["key"];

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "activate_accounts");

    Json find_account = query.find_one("key", key);

    if (find_account != nullptr)
    {
        response["data"] = "";
        response["msg"] = "activate account [" + key + "] already exist";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        std::string exchange = account["exchange"];
        Json available_account = query.find_one("exchange", exchange);

        // Insert
        if (available_account == nullptr)
        {
            query.insert_one(account);

            // Response
            response["data"] = {};
            response["msg"] = "register account [" + key + "] successfully";
            response["status_code"] = OK_200;
            response["error"] = false;
        }
        // Replace new account for the same [exchange]
        else
        {
            query.replace_one("exchange", exchange, account);

            // Response
            response["data"] = {};
            response["msg"] = "replace account [" + key + "] for exchange [" + exchange + "] successfully";
            response["status_code"] = OK_200;
            response["error"] = false;
        }

    }

    co_return HttpResponse(OK_200, response);
}