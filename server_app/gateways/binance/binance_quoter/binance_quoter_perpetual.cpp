#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>
#include <network/https_client_request/https_client_request.h>

#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>
#include <app_utils/app_utils.h>

#include <time/measure_time.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

BinanceQuoterPerpetual::BinanceQuoterPerpetual(const std::string& key)
    : BinanceQuoter(key), m_epoll_base{(EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::SYSTEM_IO_TASK)}
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
    EventBase* event_base = EventBaseManager::get_event_base_by_id(EpollBaseID::GATEWAY);

    auto task = this->get_listen_key();
    m_listen_key = task.start_running_on(event_base).get();

    m_websocket = std::make_shared<WebsocketClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), event_base);
    m_websocket->set_callbacks(
        // on_connect
        [this]() -> Task<void>
        {
            spdlog::info("BinanceQuoterPerpetual websocket connected");

            // Set period time to re-active m_listen_key at every 30 seconds
            m_websocket->add_keep_websocket_alive_task([this]() -> Task<void>
            {
                return keep_listen_key();
            }, CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD);

            co_return;
        },
        // on_message
        [this](std::string buffer) -> Task<void>
        {
            Json json = Json::parse(buffer);

            if (json["e"] == "ORDER_TRADE_UPDATE")
            {
                Json o = json["o"];

                std::string exchange_symbol = o["s"];
                const Instrument* instrument = Instrument::get_instrument_by_exchange_symbol(ExchangeId::BINANCE, InstrumentType::PERPETUAL, exchange_symbol);

                Order order
                {
                    0,                                   // Order Id
                    Order::Status::NEW,                  // Status
                    instrument,                          // Instrument
                    enum_reflect::enum_value<Order::Side>((std::string)o["S"]), // Side
                    enum_reflect::enum_value<Order::OrderType>((std::string)o["o"]), // Type
                    std::stod((std::string)o["p"]),   // Price
                    std::stod((std::string)o["q"]),   // Quantity
                };

                if (o["X"] == "NEW")
                {
                    order.order_id = AppUtils::instance().parse_order_id(o["c"]);
                    order.status = Order::Status::NEW;
                }
                else if (o["X"] == "FILLED")
                {
                    order.order_id = AppUtils::instance().parse_order_id(o["c"]);
                    order.status = Order::Status::FILLED;
                    order.filled_quantity = std::stod((std::string)o["l"]);
                    order.filled_price = std::stod((std::string)o["L"]);
                    order.fee = std::stod((std::string)o["n"]);
                    order.commission_asset = (std::string)o["N"];
                }
                else if (o["X"] == "PARTIALLY_FILLED")
                {
                    order.order_id = AppUtils::instance().parse_order_id(o["c"]);
                    order.status = Order::Status::PARTIALLY_FILLED;
                    order.filled_quantity = std::stod((std::string)o["l"]);
                    order.filled_price = std::stod((std::string)o["L"]);
                    order.fee = std::stod((std::string)o["n"]);
                    order.commission_asset = (std::string)o["N"];
                }
                else if (o["X"] == "CANCELED")
                {
                    order.order_id = AppUtils::instance().parse_order_id(o["c"]);
                    order.status = Order::Status::CANCELED;
                }

                // spdlog::info("BinanceQuoterPerpetual Order: {}", order.to_json());

                // Only update order if [order.order_id] != 0
                if (order.order_id != 0)
                {
                    OrderManager::instance().update_order(order);
                }
            }

            co_return;
        },
        // on_disconnect
        [this]() -> Task<void>
        {
            // Re-start
            spdlog::info("BinanceQuoterPerpetual - disconnect, re-starting");
            this->init_websocket();

            co_return;
        },
        // on_close
        []() -> Task<void>
        {
            spdlog::info("BinanceQuoterPerpetual - close");
            co_return;
        }
    );

    m_websocket->connect(m_ws_url, m_ws_port, "/ws/" + m_listen_key);
}

Task<std::string> BinanceQuoterPerpetual::get_listen_key()
{
    HttpsClientRequest client(m_epoll_base, m_url, std::stoi(m_port));
    client.add_header("X-MBX-APIKEY", m_api_key);

    HttpsClientResponse response = co_await client.post("/fapi/v1/listenKey", "");
    Json data = Json::parse(response.body);

    if (data.has_field("code") && (double)data["code"] < 0)
    {
        spdlog::error("BinanceQuoterPerpetual - Error fetching listen key: {}", data);
    }
    else
    {
        spdlog::debug("BinanceQuoterPerpetual - listenKey: {}", data);
    }

    co_return data["listenKey"];
}

Task<void> BinanceQuoterPerpetual::keep_listen_key()
{
    HttpsClientRequest client(m_epoll_base, m_url, std::stoi(m_port));
    client.add_header("X-MBX-APIKEY", m_api_key);

    HttpsClientResponse response = co_await client.put("/fapi/v1/listenKey?listenKey=" + m_listen_key, "");
    Json data = Json::parse(response.body);

    spdlog::debug("BinanceQuoterPerpetual, re-active m_listen_key: {}", data);

    // Send ping
    m_websocket->send_ping();
}

Task<Json> BinanceQuoterPerpetual::get_open_orders(std::string symbol)
{
    co_return co_await send_binance_request(RequestMethod::GET, "fapi/v1/openOrders", "symbol=" + symbol);
}

Task<void> BinanceQuoterPerpetual::cancel_all(std::string symbol)
{
    co_await send_binance_request(RequestMethod::DELETE, "/fapi/v1/allOpenOrders", "symbol=" + symbol);
    co_return;
}

Task<Json> BinanceQuoterPerpetual::cancel(Order order)
{
    // DELETE /api/v3/order?symbol=BTCUSDT&origClientOrderId=my_custom_id_123&timestamp=1743540000000&signature=abcdef
    std::string query_str;

    spdlog::debug("cancel order: {}", order.to_json());

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&origClientOrderId=" + std::to_string(order.order_id);

    co_return co_await send_binance_request(RequestMethod::DELETE, "/fapi/v1/order", std::move(query_str));
}

Task<Json> BinanceQuoterPerpetual::place(Order order)
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
        query_str += "&timeInForce=GTX";
        query_str += "&price=" + std::to_string(order.price);
    }

    co_return co_await send_binance_request(RequestMethod::POST, "/fapi/v1/order", std::move(query_str));
}