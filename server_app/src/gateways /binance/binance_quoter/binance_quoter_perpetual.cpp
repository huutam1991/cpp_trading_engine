#include <external_request/external_request_ssl.h>
#include <timer.h>

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
    del_timer_keep_alive_listen_key();
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
    m_listen_key = this->get_listen_key();

    // Set period time to re-active m_listen_key at every 30 minutes (1800 seconds)
    add_timer_keep_alive_listen_key(1800000);

    m_websocket = std::make_shared<WebsocketClient>(m_ws_url, m_ws_port, "/ws/" + m_listen_key);

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("BinanceQuoterPerpetual websocket connected");
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        // ADD_LOG("BinanceFuturesUser OnMessage: " << buffer);
        Json json = Json::parse(buffer);
        ADD_LOG("BinanceFuturesUser on_message: " << json);

        if (json.has_field("e") == true)
        {
            std::string event_type = json["e"];
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("BinanceFuturesUser on_close, close_code = " << close_code);

        // Unexpected close, need to re-start
        if (close_code == websocket::close_code::internal_error)
        {
            // Delete current schedule task to re-active m_listen_key
            this->del_timer_keep_alive_listen_key();

            // Re-start
            ADD_LOG("BinanceFutures - re-starting");
            this->init_websocket();
        }
    });

    m_websocket->run();
}

std::string BinanceQuoterPerpetual::get_listen_key()
{
    ExternalRequestSsl binance_request(m_url, m_port, "/fapi/v1/listenKey", RequestMethod::POST);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    std::string res = binance_request.send_request("");
    Json data = Json::parse(res);
    return data["listenKey"];
}

void BinanceQuoterPerpetual::add_timer_keep_alive_listen_key(size_t period)
{
    m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    {
        ExternalRequestSsl binance_request(m_url, "443", "/fapi/v1/listenKey?listenKey=" + m_listen_key, RequestMethod::PUT);
        binance_request.add_header("X-MBX-APIKEY", m_api_key);

        ADD_LOG("BinanceQuoterPerpetual re-active m_listen_key = " << m_listen_key);

        std::string res = binance_request.send_request("");
        Json data = Json::parse(res);
        ADD_LOG("re-active data: " << data.get_string_value());
    },
    period);
}

void BinanceQuoterPerpetual::del_timer_keep_alive_listen_key()
{
    if (m_schedule_task_id != 0)
    {
        Timer::instance().delete_schedule_task(this->m_schedule_task_id);
    }
}

Json BinanceQuoterPerpetual::get_trade_result_from_response(Json& response)
{
    // Get [symbol] + [quantity]
    std::string symbol;
    double quantity = 0;
    double volumn_in_usdt = 0;
    double price = response["price"];

    // Get fill symbol + quantity
    if (response.has_field("symbol") && response.has_field("origQty"))
    {
        symbol = std::string(response["symbol"]);
        quantity = std::stod(std::string(response["origQty"]));
        std::string side = std::string(response["side"]);

        // Get volumn in USDT
        volumn_in_usdt = quantity * price;

        // Use BUY to take profit or stop loss, so symbol should be USDT
        if (side == "BUY")
        {
            symbol = "USDT";
        }

        ADD_LOG("Perpetual order place - symbol: " << symbol << ", quantity: " << quantity << ", volumn_in_usdt: " << volumn_in_usdt);
    }

    return {
        {"type", "perpetual"},
        {"symbol", symbol},
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt}
    };
}

Json BinanceQuoterPerpetual::place(Order order)
{
    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;
    std::string side = order.side == Order::Side::BUY ? "BUY" : "SELL";

    query_str += "symbol=" + order.symbol;
    query_str += "&side=" + side;
    query_str += "&type=" + order.type;
    query_str += "&quantity=" + std::to_string(order.quantity);

    if (order.type == "LIMIT")
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    return send_binance_request(RequestMethod::POST, "/fapi/v1/order", query_str);
}