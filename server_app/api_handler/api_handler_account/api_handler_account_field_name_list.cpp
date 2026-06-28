#include <api_handler/api_handler_account/api_handler_account_field_name_list.h>
#include <account/account.h>

APIHandlerAccountFieldNameList::APIHandlerAccountFieldNameList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerAccountFieldNameList::child_handle()
{
    Json response;

    response["data"] = AccountManager::get_all_accounts_field_names();
    response["msg"] = "Get account field name list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}