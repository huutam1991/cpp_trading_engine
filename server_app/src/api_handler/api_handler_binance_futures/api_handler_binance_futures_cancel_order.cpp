#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_cancel_order.h>
#include <utils.h>

APIHandlerBinanceFuturesCancelOrder::APIHandlerBinanceFuturesCancelOrder(HttpRequest* request) : APIHandlerBinanceFutures(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
    add_mandatory_body_params({"symbol", "order_ids"});
}

Json APIHandlerBinanceFuturesCancelOrder::send_cancel_order_request(const std::string& symbol, long orderId)
{
    Json query_json;
    if (m_request != nullptr)
    {
        query_json = m_request->get_query_json();
    }

    query_json["orderId"] = orderId;
    query_json["symbol"] = symbol;

    std::string query_string = m_request->get_query_string_from_query_json(query_json);
    return send_binance_request("/fapi/v1/order", query_string, RequestMethod::DELETE);
}

Json APIHandlerBinanceFuturesCancelOrder::handle_internal_request(const std::string& symbol, long orderId)
{
    const std::string user_id = "root";
    APIHandlerBinanceFuturesCancelOrder api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.send_cancel_order_request(symbol, orderId);
}

HttpResponse APIHandlerBinanceFuturesCancelOrder::child_handle()
{
    Json cancel_result = Json::create_array();
    Json orderId_list = m_request->get_body_param_json("order_ids");
    std::string symbol = m_request->get_body_param_string("symbol");
    bool is_there_success = false;

    orderId_list.for_each([&cancel_result, &symbol, &is_there_success, this](Json& data)
    {
        long orderId = data;

        Json cancel_data = send_cancel_order_request(symbol, orderId);
        if (cancel_data.has_field("code") && ((long)cancel_data["code"]) < -1)
        {
            cancel_data = cancel_data["msg"];
        }
        else
        {
            is_there_success = true;
        }

        cancel_result.push_back(cancel_data);
    });

    // Response to client
    Json response;
    response["data"] = cancel_result;
    response["msg"] = "";

    if (is_there_success == true)
    {
        response["status_code"] = OK_200;
        response["error"] = false;
    }
    else
    {
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }

    return HttpResponse((ResponseStatusCode)response["status_code"], response);
}