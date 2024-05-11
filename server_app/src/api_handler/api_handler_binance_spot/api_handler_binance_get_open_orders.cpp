#include <api_handler/api_handler_binance_spot/api_handler_binance_get_open_orders.h>
#include <utils.h>

APIHandlerBinanceGetOpenOrders::APIHandlerBinanceGetOpenOrders(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
    add_mandatory_params({"symbol"});
}

Json APIHandlerBinanceGetOpenOrders::handle_internal_request(const std::string& symbol, const std::string user_id)
{
    APIHandlerBinanceGetOpenOrders api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.send_get_open_orders_request(symbol);
}

Json APIHandlerBinanceGetOpenOrders::send_get_open_orders_request(const std::string& symbol)
{
    return send_binance_request("/api/v3/openOrders", "symbol=" + symbol, RequestMethod::GET);
}

HttpResponse APIHandlerBinanceGetOpenOrders::child_handle()
{
    // /api/v3/openOrders?symbol=ETHBTC&recvWindow=15000
    // &timestamp={{timestamp}}&signature={{signature}}

    std::string symbol = m_request->get_query_param("symbol");
    Json open_order_ids = Json::create_array();
    Json open_orders = send_get_open_orders_request(symbol);

    open_orders.for_each([&open_order_ids](Json& order)
    {
        open_order_ids.push_back(order["orderId"]);
    });

    Json data = {
        {"z_orders", open_orders},
        {"order_ids", open_order_ids}
    };

    // Response to client
    Json response;
    response["data"] = data;
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(ResponseStatusCode::OK_200, response);
}