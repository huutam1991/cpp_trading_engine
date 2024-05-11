
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_get_open_orders.h>
#include <utils.h>

APIHandlerBinanceFuturesGetOpenOrders::APIHandlerBinanceFuturesGetOpenOrders(HttpRequest* request) : APIHandlerBinanceFutures(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
    add_mandatory_params({"symbol"});
}

Json APIHandlerBinanceFuturesGetOpenOrders::handle_internal_request(const std::string& symbol, const std::string user_id)
{
    APIHandlerBinanceFuturesGetOpenOrders api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.send_get_open_orders_request(symbol);
}

Json APIHandlerBinanceFuturesGetOpenOrders::send_get_open_orders_request(const std::string& symbol)
{
    return send_binance_request("/fapi/v1/openOrders", "symbol=" + symbol, RequestMethod::GET);
}

HttpResponse APIHandlerBinanceFuturesGetOpenOrders::child_handle()
{
    // /api/v3/openOrders?symbol=ETHBTC&recvWindow=15000
    // &timestamp={{timestamp}}&signature={{signature}}

    std::string symbol = m_request->get_query_param("symbol");

    // Response to client
    Json response;
    response["data"] = send_get_open_orders_request(symbol);
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(ResponseStatusCode::OK_200, response);
}