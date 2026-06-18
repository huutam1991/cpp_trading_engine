#include <time/measure_time.h>

#include <gateways/coinbase/coinbase_quoter/coinbase_quoter_spot.h>
#include <app_utils/app_utils.h>

CoinbaseQuoterSpot::CoinbaseQuoterSpot(const std::string& key) : CoinbaseQuoter(key)
{
    m_url = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_URL : COINBASE_ADVANCE_REALNET_URL;
    m_port = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_PORT : COINBASE_ADVANCE_REALNET_PORT;

    // websocket
    m_ws_url = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_WS_URL : COINBASE_ADVANCE_REALNET_WS_URL;
    m_ws_port = m_is_testnet == true ? COINBASE_ADVANCE_REALNET_WS_PORT : COINBASE_ADVANCE_REALNET_WS_PORT;

    // Event base: GATEWAY
    m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY);

    init_websocket();
}

CoinbaseQuoterSpot::~CoinbaseQuoterSpot()
{
}

std::string& CoinbaseQuoterSpot::get_url()
{
    return m_url;
}

std::string& CoinbaseQuoterSpot::get_port()
{
    return m_port;
}

void CoinbaseQuoterSpot::init_websocket()
{
    // Get listen key
    auto task = this->get_listen_key();
    m_listen_key = task.start_running_on(m_event_base).get();
}

Task<std::string> CoinbaseQuoterSpot::get_listen_key()
{
    // RequestFuture coinbase_request(m_url, m_port, "/api/v3/userDataStream", RequestMethod::POST);
    // coinbase_request.add_header("X-MBX-APIKEY", m_api_key);

    // Json data = co_await coinbase_request.send_request();
    // co_return data["listenKey"];

    co_return "listenKey";
}

Task<void> CoinbaseQuoterSpot::keep_listen_key()
{
    // RequestFuture coinbase_request(m_url, m_port, "/api/v3/userDataStream?listenKey=" + m_listen_key, RequestMethod::PUT);
    // coinbase_request.add_header("X-MBX-APIKEY", m_api_key);
    // co_await coinbase_request.send_request();

    // // Send ping
    // m_websocket->send_ping();

    co_return;
}

Json CoinbaseQuoterSpot::get_trade_result_from_response(Json& response)
{
    // Return empty data if has error
    if ((long)response["code"] < 0)
    {
        return {
            {"type", "spot"},
            {"symbol", response["symbol"]},
            {"quantity", 0.0},
            {"volumn_in_usdt", 0.0}
        };
    }

    // Get [symbol] + [quantity]
    std::string symbol;
    double quantity = 0;
    double volumn_in_usdt = 0;

    // Get fill symbol + quantity
    if (response.has_field("fills"))
    {
        Json fills = response["fills"];

        fills.for_each([&](Json& fill)
        {
            double f_quantity = std::stod(std::string(fill["qty"]));
            double f_commission = std::stod(std::string(fill["commission"]));
            double f_price = std::stod(std::string(fill["price"]));
            symbol = std::string(fill["commissionAsset"]);

            // This is BUY order
            if (symbol == "BTC" || symbol == "ETH" || symbol == "SOL")
            {
                quantity += f_quantity - f_commission;
                volumn_in_usdt += f_quantity * f_price;
            }
            // This is SELL order
            else
            {
                volumn_in_usdt += (f_quantity * f_price) - f_commission;
            }
        });
    }

    return {
        {"type", "spot"},
        {"symbol", symbol},
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt}
    };
}

Task<Json> CoinbaseQuoterSpot::get_open_orders(std::string symbol)
{
    co_return co_await send_coinbase_request(RequestMethod::GET, "/api/v3/openOrders", "symbol=" + symbol);
}

Task<void> CoinbaseQuoterSpot::cancel_all(std::string symbol)
{
    co_await send_coinbase_request(RequestMethod::DELETE, "/api/v3/openOrders", "symbol=" + symbol);

    co_return;
}

Task<Json> CoinbaseQuoterSpot::cancel(Order order)
{
    // DELETE /api/v3/order?symbol=BTCUSDT&origClientOrderId=my_custom_id_123&timestamp=1743540000000&signature=abcdef
    std::string query_str;

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&origClientOrderId=" + std::to_string(order.order_id);

    co_return co_await send_coinbase_request(RequestMethod::DELETE, "/api/v3/order", std::move(query_str));
}

Task<Json> CoinbaseQuoterSpot::place(Order order)
{
    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&side=" + (std::string)enum_reflect::enum_name(order.side);
    query_str += "&type=" + (std::string)enum_reflect::enum_name(order.type);
    query_str += "&quantity=" + std::to_string(order.quantity);
    query_str += "&newClientOrderId=" + std::to_string(order.order_id);

    if (order.type == Order::OrderType::LIMIT)
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    co_return co_await send_coinbase_request(RequestMethod::POST, "/api/v3/order", std::move(query_str));
}
