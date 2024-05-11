#include <api_handler/api_handler_coinbase/api_handler_coinbase_cancel_order.h>

APIHandlerCoinbaseCancelOrder::APIHandlerCoinbaseCancelOrder(HttpRequest* request) : APIHandlerCoinbase(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;

    add_mandatory_params({"orderId"});
}

HttpResponse APIHandlerCoinbaseCancelOrder::child_handle()
{
    std::string orderId = m_request->get_query_param("orderId");

    Json response = {
        {"data", send_coinbase_request("/orders", orderId, "", RequestMethod::DELETE)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}