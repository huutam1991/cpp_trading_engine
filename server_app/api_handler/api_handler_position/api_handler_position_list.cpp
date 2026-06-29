#include <api_handler/api_handler_position/api_handler_position_list.h>
#include <account/account_manager.h>
#include <account/account.h>

APIHandlerPositionList::APIHandlerPositionList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerPositionList::child_handle()
{
    Json positions;

    auto active_accounts = AccountManager::get_active_accounts();
    for (std::shared_ptr<AccountBase> account : active_accounts)
    {
        Json account_positions = co_await account->m_order_entry->get_positions();

        account_positions.for_each([&](Json& position)
        {
            positions.push_back(position);
        });
    }

    // Response
    Json response;
    // [Tam temporarily comment out - OrderEntry refactor]
    // response["positions"] = co_await gateway_binance->get_positions();
    response["positions"] = positions;
    response["msg"] = "Get position list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;


    co_return HttpResponse(OK_200, response);
}
