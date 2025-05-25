#include <external_request/external_request_ssl.h>
#include <timer.h>
#include <ioc_pool.h>

#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>

BinanceQuoterPerpetual::BinanceQuoterPerpetual(const std::string& key) : BinanceQuoter(key)
{
    m_url = m_is_testnet == true ? BINANCE_TESTNET_FUTURES_URL : BINANCE_FUTURES_URL;
    m_port = m_is_testnet == true ? BINANCE_TESTNET_FUTURES_PORT : BINANCE_FUTURES_PORT;

    // websocket
    m_ws_url = m_is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_URL : BINANCE_FUTURES_WS_URL;
    m_ws_port = m_is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_PORT : BINANCE_FUTURES_WS_PORT;
    init_websocket();
}

BinanceQuoterPerpetual::~BinanceQuoterPerpetual()
{
}

std::string& BinanceQuoterPerpetual::get_url()
{
    return m_url;
}

std::string& BinanceQuoterPerpetual::get_port()
{
    return m_port;
}

void BinanceQuoterPerpetual::init_websocket()
{
    if (m_websocket != nullptr)
    {
        m_websocket->close();
        m_websocket = nullptr;
    }

    // Event base: GATEWAY
    EventBase* event_base = EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY);

    m_listen_key = this->get_listen_key();

    m_websocket = std::make_shared<WebsocketClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), event_base);
    m_websocket->set_callbacks(
        // on_connect
        [this]() -> TaskVoid
        {
            ADD_LOG("BinanceQuoterPerpetual websocket connected");

            // Set period time to re-active m_listen_key at every 30 minutes (1800 seconds)
            add_timer_keep_alive_listen_key(1800000);

            co_return;
        },
        // on_message
        [this](std::string buffer) -> TaskVoid
        {
            Json json = Json::parse(buffer);

            if (json["e"] == "ORDER_TRADE_UPDATE")
            {
                Json order = json["o"];
                if (order["X"] == "FILLED")
                {
                    Json data = {
                        {"status", "FILLED"},
                        {"symbol", order["s"]},
                        {"price", std::stod(std::string(order["ap"]))},
                        {"quantity", std::stod(std::string(order["z"]))}
                    };

                    ADD_LOG("BinanceQuoterPerpetual Filled: " << data);

                    update_order_result(data);
                }
            }

            co_return;
        },
        // on_disconnect
        [this]() -> TaskVoid
        {
            // Re-start
            ADD_LOG("BinanceQuoterPerpetual - disconnect, re-starting");
            this->init_websocket();

            co_return;
        },
        // on_close
        []() -> TaskVoid
        {
            ADD_LOG("BinanceQuoterPerpetual close");
            co_return;
        }
    );

    m_websocket->connect(m_ws_url, m_ws_port, "/ws/" + m_listen_key);
}

std::string BinanceQuoterPerpetual::get_listen_key()
{
    ExternalRequestSsl binance_request(m_url, m_port, "/fapi/v1/listenKey", RequestMethod::POST);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    std::string res = binance_request.send_request();
    Json data = Json::parse(res);
    return data["listenKey"];
}

void BinanceQuoterPerpetual::add_timer_keep_alive_listen_key(size_t period)
{
    // m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    // {
    //     ExternalRequestSsl binance_request(m_url, "443", "/fapi/v1/listenKey?listenKey=" + m_listen_key, RequestMethod::PUT);
    //     binance_request.add_header("X-MBX-APIKEY", m_api_key);

    //     ADD_LOG("BinanceQuoterPerpetual re-active m_listen_key = " << m_listen_key);

    //     std::string res = binance_request.send_request();
    //     Json data = Json::parse(res);
    //     ADD_LOG("re-active data: " << data.get_string_value());
    // },
    // period);
}

void BinanceQuoterPerpetual::update_order_result(const Json& order_result)
{
    std::unique_lock lock(m_mutex);
    m_order_result = order_result;
}

Json BinanceQuoterPerpetual::get_trade_result_from_response(Json& response)
{
    // Return empty data if has error
    if ((long)response["code"] < 0)
    {
        ADD_LOG("Perpetual order error: " << response);
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

    ADD_LOG("Perpetual order place - symbol: " << symbol << ", quantity: " << quantity << ", volumn_in_usdt: " << volumn_in_usdt);

    return {
        {"type", "perpetual"},
        {"symbol", symbol},
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt}
    };
}

Task<Json> BinanceQuoterPerpetual::get_open_orders(std::string symbol)
{
    co_return co_await send_binance_request(RequestMethod::GET, "fapi/v1/openOrders", "symbol=" + symbol);
}

TaskVoid BinanceQuoterPerpetual::cancel_all(std::string symbol)
{
    co_await send_binance_request(RequestMethod::DELETE, "/fapi/v1/allOpenOrders", "symbol=" + symbol);
    co_return;
}

Task<Json> BinanceQuoterPerpetual::cancel(Order order)
{
    // Need to implement later (current code is wrong)
    co_return co_await send_binance_request(RequestMethod::DELETE, "/fapi/v1/allOpenOrders", "");
}

Task<Json> BinanceQuoterPerpetual::place(Order order)
{
    // Update order result to "placing", mean need to wait until it get filled
    update_order_result({
        {"status", "PLACING"},
    });

    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;

    query_str += "symbol=" + order.symbol;
    query_str += "&side=" + Order::to_string(order.side);
    query_str += "&type=" + Order::to_string(order.type);
    query_str += "&quantity=" + std::to_string(order.quantity);

    if (order.type == Order::OrderType::LIMIT)
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    co_return co_await send_binance_request(RequestMethod::POST, "/fapi/v1/order", std::move(query_str));
}