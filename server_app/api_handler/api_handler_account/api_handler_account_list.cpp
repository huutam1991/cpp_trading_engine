#include <api_handler/api_handler_account/api_handler_account_list.h>
#include <account/account_db.h>

APIHandlerAccountList::APIHandlerAccountList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerAccountList::child_handle()
{
    Json response;

    response["data"] = AccountDB::load_all_accounts();
    response["msg"] = "Get account list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}