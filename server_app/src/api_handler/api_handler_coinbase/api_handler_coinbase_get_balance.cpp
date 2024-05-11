#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_balance.h>
#include <utils.h>

APIHandlerCoinbaseGetBalance::APIHandlerCoinbaseGetBalance(HttpRequest* request) : APIHandlerCoinbase(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

HttpResponse APIHandlerCoinbaseGetBalance::child_handle()
{
    std::string symbol = m_request->get_query_param("symbol");

    Json accounts_info = send_coinbase_request("/accounts", "", "", RequestMethod::GET);
    ADD_LOG("Coinbase accounts: " << accounts_info);
    Json balance_by_symbol;
    accounts_info.for_each([&symbol, &balance_by_symbol](Json& data)
    {
        data.remove_field("trading_enabled");
        data.remove_field("id");

        // If search by symbol
        std::string currency = data["currency"];
        if (symbol == currency)
        {
            balance_by_symbol = data;
        }
    });

    Json response = {
        {"data", symbol == PARAM_NOT_FOUND ? accounts_info : balance_by_symbol},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}