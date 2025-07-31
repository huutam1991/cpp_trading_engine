#include <coroutine/event_base_manager.h>
#include <api_handler/api_handler_account/api_handler_activate_account_balances.h>
#include <gateways/gateway_manager.h>

#include <app_utils/app_utils.h>

#include <set>

APIHandlerActivateAccountBalances::APIHandlerActivateAccountBalances(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"symbols"});
}

Task<HttpResponse> APIHandlerActivateAccountBalances::child_handle()
{
    // Get [symbols_set] from request
    std::set<std::string> symbols_set;
    JsonNew symbols = m_request->get_body_param_json("symbols");
    symbols.for_each([&symbols_set](JsonNew& symbol)
    {
        symbols_set.insert(std::string(symbol));
    });

    // Get activate account's balances
    JsonNew activate_accounts = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "activate_accounts")
        .find_many();
    std::string exchange = activate_accounts[0]["exchange"];

    // Use coroutine
    JsonNew balances = co_await GatewayManager::instance().get_gateway(exchange)->get_balances();
    // spdlog::debug("balances: {}", balances);

    // Form response data from [balances] + [symbols_set]
    JsonNew data;
    balances.for_each([&symbols_set, &data](JsonNew& balance)
    {
        std::string asset = balance["asset"];

        if (symbols_set.find(asset) != symbols_set.end())
        {
            data[asset] = balance["available"];
        }
    });

    // Response
    JsonNew response;
    response["data"] = data;
    response["msg"] = "activate account of exchange [" + exchange + "] balances";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}