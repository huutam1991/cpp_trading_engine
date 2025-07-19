#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>

#include <gateways/coinbase/coinbase_quoter/coinbase_quoter_perpetual.h>

CoinbaseQuoterPerpetual::CoinbaseQuoterPerpetual(const std::string& key) : CoinbaseQuoter(key)
{
    m_url = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_URL : COINBASE_ADVANCE_REALNET_URL;
    m_port = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_PORT : COINBASE_ADVANCE_REALNET_PORT;

    // websocket
    m_ws_url = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_WS_URL : COINBASE_ADVANCE_REALNET_WS_URL;
    m_ws_port = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_WS_PORT : COINBASE_ADVANCE_REALNET_WS_PORT;
    init_websocket();
}

CoinbaseQuoterPerpetual::~CoinbaseQuoterPerpetual()
{
}

std::string& CoinbaseQuoterPerpetual::get_url()
{
    return m_url;
}

std::string& CoinbaseQuoterPerpetual::get_port()
{
    return m_port;
}

void CoinbaseQuoterPerpetual::init_websocket()
{
    
}

std::string CoinbaseQuoterPerpetual::get_listen_key()
{
    return {};
}

void CoinbaseQuoterPerpetual::update_order_result(const Json& order_result)
{
    std::unique_lock lock(m_mutex);
    m_order_result = order_result;
}

Json CoinbaseQuoterPerpetual::get_trade_result_from_response(Json& response)
{
    // Return empty data if has error
    if ((long)response["code"] < 0)
    {
        return {
            {"type", "perpetual"},
            {"symbol", response["symbol"]},
            {"quantity", 0.0},
            {"volumn_in_usdt", 0.0}
        };
    }

    // Tricky here
    while (m_order_result["status"] == "PLACING")
    {
    }

    // Get [symbol] + [quantity]
    std::string symbol = m_order_result["symbol"];
    double quantity = m_order_result["quantity"];
    double price = m_order_result["price"];
    double volumn_in_usdt = quantity * price;

    return {
        {"type", "perpetual"},
        {"symbol", symbol},
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt}
    };
}

Task<Json> CoinbaseQuoterPerpetual::get_open_orders(std::string symbol)
{
    co_return co_await send_coinbase_request(RequestMethod::GET, "fapi/v1/openOrders", "symbol=" + symbol);
}

TaskVoid CoinbaseQuoterPerpetual::cancel_all(std::string symbol)
{
    co_await send_coinbase_request(RequestMethod::DELETE, "/fapi/v1/allOpenOrders", "symbol=" + symbol);
    co_return;
}

Task<Json> CoinbaseQuoterPerpetual::cancel(Order order)
{
    // Need to implement later (current code is wrong)
    co_return co_await send_coinbase_request(RequestMethod::DELETE, "/fapi/v1/allOpenOrders", "");
}

Task<Json> CoinbaseQuoterPerpetual::place(Order order)
{
    // Update order result to "placing", mean need to wait until it get filled
    update_order_result({
        {"status", "PLACING"},
    });

    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&side=" + (std::string)enum_reflect::enum_name(order.side);
    query_str += "&type=" + (std::string)enum_reflect::enum_name(order.type);
    query_str += "&quantity=" + std::to_string(order.quantity);

    if (order.type == Order::OrderType::LIMIT)
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    co_return co_await send_coinbase_request(RequestMethod::POST, "/fapi/v1/order", std::move(query_str));
}