#include <api_handler/api_handler_binance_spot/api_handler_binance_order_allow_cancel_stop_loss.h>

APIHandlerBinanceOrderAllowCancelStopLoss::APIHandlerBinanceOrderAllowCancelStopLoss(HttpRequest* request) : APIHandlerBinanceCancelReplaceOrder(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

std::string APIHandlerBinanceOrderAllowCancelStopLoss::get_query(Json& query_json)
{
    return m_request->get_query_string_from_query_json(query_json) +
           "&type=STOP_LOSS_LIMIT&timeInForce=GTC"; // type=STOP_LOSS + timeInForce=GTC is default
}

Json APIHandlerBinanceOrderAllowCancelStopLoss::handle_internal_request(const std::string& user_id, Json& query_json, Json& price_ticker)
{
    APIHandlerBinanceOrderAllowCancelStopLoss api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.handle_send_order_request(query_json);
}

