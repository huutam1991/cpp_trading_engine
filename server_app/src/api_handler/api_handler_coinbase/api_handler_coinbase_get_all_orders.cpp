#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_all_orders.h>
#include <utils.h>

APIHandlerCoinbaseGetAllOrders::APIHandlerCoinbaseGetAllOrders(HttpRequest* request) : APIHandlerCoinbase(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

HttpResponse APIHandlerCoinbaseGetAllOrders::child_handle()
{
    Json response = {
        {"data", send_coinbase_request("/orders", "", "", RequestMethod::GET)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}