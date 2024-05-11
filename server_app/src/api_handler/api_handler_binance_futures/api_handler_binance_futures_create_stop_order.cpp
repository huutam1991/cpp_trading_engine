
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_create_stop_order.h>

APIHandlerBinanceFuturesCreateStopOrder::APIHandlerBinanceFuturesCreateStopOrder(HttpRequest* request) : APIHandlerBinanceFuturesCreateOrder(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

std::string APIHandlerBinanceFuturesCreateStopOrder::get_query_string(Json& query_json)
{
    return m_request->get_query_string_from_query_json(query_json) +
           "&type=STOP&timeInForce=GTC"; // type=STOP + timeInForce=GTC is default
}

Json APIHandlerBinanceFuturesCreateStopOrder::handle_internal_request(const std::string& user_id, Json& query_json, Json& price_ticker)
{
    APIHandlerBinanceFuturesCreateStopOrder api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.handle_send_order_request(query_json);
}