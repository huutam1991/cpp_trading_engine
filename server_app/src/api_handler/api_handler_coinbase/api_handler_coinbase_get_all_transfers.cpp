#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_all_transfers.h>
#include <utils.h>

APIHandlerCoinbaseGetAllTransfers::APIHandlerCoinbaseGetAllTransfers(HttpRequest* request) : APIHandlerCoinbase(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;

    add_mandatory_params({"profile_id"});
}

HttpResponse APIHandlerCoinbaseGetAllTransfers::child_handle()
{
    std::string profile_id = m_request->get_query_param("profile_id");

    Json response = {
        {"data", send_coinbase_request("/transfers", "profile_id=" + profile_id, "", RequestMethod::GET)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}