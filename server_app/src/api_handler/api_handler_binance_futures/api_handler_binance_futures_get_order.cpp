
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_get_order.h>

APIHandlerBinanceFuturesGetOrder::APIHandlerBinanceFuturesGetOrder(HttpRequest* request) : APIHandlerBinanceFutures(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
    add_mandatory_params({"symbol", "order_id"});
}

Json APIHandlerBinanceFuturesGetOrder::send_get_order_info_request(const std::string& symbol, long orderId)
{
    Json query_json;
    Json request_json;
    if (m_request != nullptr)
    {
        request_json = m_request->get_query_json();

        // query_json["token"] = request_json["token"];
    }

    query_json["orderId"] = orderId;
    query_json["symbol"] = symbol;

    std::string query_string = m_request->get_query_string_from_query_json(query_json);
    return send_binance_request("/fapi/v1/order", query_string, RequestMethod::GET);
}

Json APIHandlerBinanceFuturesGetOrder::handle_internal_request(const std::string& symbol, long orderId, const std::string user_id)
{
    APIHandlerBinanceFuturesGetOrder api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.send_get_order_info_request(symbol, orderId);
}

HttpResponse APIHandlerBinanceFuturesGetOrder::child_handle()
{
    // /api/v3/order?symbol=ETHUSDT&orderId=9194425
    // &timestamp={{timestamp}}&signature={{signature}}

    std::string symbol = m_request->get_query_param("symbol");
    long order_id = std::stol(m_request->get_query_param("order_id"));

    Json get_info_respose = send_get_order_info_request(symbol, order_id);

    Json response;

    if (get_info_respose.has_field("code") && ((long)get_info_respose["code"]) < -1)
    {
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
        response["msg"] = get_info_respose["msg"];
        response["data"] = Json::create_array();
    }
    else
    {
        response["status_code"] = OK_200;
        response["error"] = false;
        response["msg"] = "";
        response["data"] = get_info_respose;
    }

    return HttpResponse((ResponseStatusCode)response["status_code"], response);
}
