#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_all_fills.h>
#include <utils.h>

APIHandlerCoinbaseGetAllFills::APIHandlerCoinbaseGetAllFills(HttpRequest* request) : APIHandlerCoinbase(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
    add_mandatory_params({"product_id"});
}

HttpResponse APIHandlerCoinbaseGetAllFills::child_handle()
{
    std::string product_id = m_request->get_query_param("product_id");

    Json response = {
        {"data", send_coinbase_request("/fills", "product_id=" + product_id, "", RequestMethod::GET)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}