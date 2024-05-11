#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_replace_order.h>

APIHandlerBinanceCancelReplaceOrder::APIHandlerBinanceCancelReplaceOrder(HttpRequest* request) : APIHandlerBinanceCreateOrder(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinanceCancelReplaceOrder::send_new_order_request(Json& query_json)
{
    handle_query_json(query_json);

    // Cancel order id
    // std::string cancel_order_id = query_json["cancel_order_id"];
    // query_json.remove_field("cancel_order_id");

    // std::string query_str = get_query(query_json) +
    //                         "&cancelReplaceMode=STOP_ON_FAILURE&cancelOrderId=" + cancel_order_id;
    std::string query_str = get_query(query_json) +
                            "&cancelReplaceMode=STOP_ON_FAILURE";

    return send_binance_request("/api/v3/order/cancelReplace", query_str, RequestMethod::POST);
}

std::string APIHandlerBinanceCancelReplaceOrder::get_query(Json& query_json)
{
    std::string type = "LIMIT";
    if (query_json.has_field("type"))
    {
        type = (std::string&&)query_json["type"];
        query_json.remove_field("type");
    }

    // timeInForce=GTC is only for LIMIT or STOP_LOSS_LIMIT
    return m_request->get_query_string_from_query_json(query_json) +
           "&type=" + type +
           ((type == "LIMIT" || type == "STOP_LOSS_LIMIT") ? "&timeInForce=GTC" : ""); 
}

Json APIHandlerBinanceCancelReplaceOrder::handle_internal_request(Json& query_json, const std::string user_id)
{
    // thread safe
    static std::mutex internal_request_mutex;
    std::unique_lock lock(internal_request_mutex);

    APIHandlerBinanceCancelReplaceOrder api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    SourceType type = StorageSource::get_source_type_by_name(BINANCE_SPOT_DB_SOURCE_NAME);
    api_handle.m_user->set_active_storage_source(type);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    // return api_handle.handle_send_order_request(query_json, price_ticker);
    return api_handle.send_new_order_request(query_json);
}

