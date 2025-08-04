#include <external_request/https_client_async.h>
#include <ioc_pool.h>
#include <time/timer.h>
#include <time/measure_time.h>
#include <ioc_pool.h>

#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <app_utils/app_utils.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

BinanceQuoterSpot::BinanceQuoterSpot(const std::string& key) : BinanceQuoter(key)
{
    m_url = m_is_testnet == true ? BINANCE_TESTNET_SPOT_URL : BINANCE_SPOT_URL;
    m_port = m_is_testnet == true ? BINANCE_TESTNET_SPOT_PORT : BINANCE_SPOT_PORT;

    // websocket
    m_ws_url = m_is_testnet == true ? BINANCE_TESTNET_SPOT_WS_URL : BINANCE_SPOT_WS_URL;
    m_ws_port = m_is_testnet == true ? BINANCE_TESTNET_SPOT_WS_PORT : BINANCE_SPOT_WS_PORT;

    // Event base: GATEWAY
    m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY);

    init_websocket();
}

BinanceQuoterSpot::~BinanceQuoterSpot()
{
}

std::string& BinanceQuoterSpot::get_url()
{
    return m_url;
}

std::string& BinanceQuoterSpot::get_port()
{
    return m_port;
}

void BinanceQuoterSpot::init_websocket()
{
    if (m_websocket != nullptr)
    {
        m_websocket->close();
        m_websocket = nullptr;
    }

    // Get listen key
    auto task = this->get_listen_key();
    m_listen_key = task.start_running_on(m_event_base).get();

    m_websocket = std::make_shared<WebsocketClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), m_event_base);
    m_websocket->set_callbacks(
        // on_connect
        [this]() -> Task<void>
        {
            spdlog::info("BinanceQuoterSpot websocket connected");

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
            // MeasureTime a("Handle order data", MeasureUnit::MICROSECOND);
            Json json = Json::parse(buffer);

            if (json["e"] == "executionReport")
            {
                std::string exchange_symbol = json["s"];
                const Instrument* instrument = Instrument::get_instrument_by_exchange_symbol(ExchangeId::BINANCE, InstrumentType::SPOT, exchange_symbol);

                Order order
                {
                    0,                                   // Order Id
                    InstrumentType::SPOT,                // Instrument Type
                    Order::Status::NEW,                  // Status
                    instrument,                          // Instrument
                    enum_reflect::enum_value<Order::Side>((std::string)json["S"]), // Side
                    enum_reflect::enum_value<Order::OrderType>((std::string)json["o"]), // Type
                    std::stod((std::string)json["p"]),   // Price
                    std::stod((std::string)json["q"]),   // Quantity
                };

                // Parsing order from execution report
                if (json["X"] == "NEW")
                {
                    order.order_id = AppUtils::instance().parse_order_id(json["c"]);
                    order.status = Order::Status::NEW;
                }
                else if (json["X"] == "FILLED")
                {
                    order.order_id = AppUtils::instance().parse_order_id(json["c"]);
                    order.status = Order::Status::FILLED;
                    order.filled_quantity = std::stod((std::string)json["l"]);
                    order.filled_price = std::stod((std::string)json["L"]);
                    order.commission_amount = std::stod((std::string)json["n"]);
                    order.commission_asset = (std::string)json["N"];
                }
                else if (json["X"] == "PARTIALLY_FILLED")
                {
                    order.order_id = AppUtils::instance().parse_order_id(json["c"]);
                    order.status = Order::Status::PARTIALLY_FILLED;
                    order.filled_quantity = std::stod((std::string)json["l"]);
                    order.filled_price = std::stod((std::string)json["L"]);
                    order.commission_amount = std::stod((std::string)json["n"]);
                    order.commission_asset = (std::string)json["N"];
                }
                else if (json["X"] == "CANCELED")
                {
                    order.order_id = AppUtils::instance().parse_order_id(json["C"]);
                    order.status = Order::Status::CANCELED;
                }

                // spdlog::info("BinanceQuoterSpot Order: {}", order.to_json());

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
            // Save when websocket spot disconnect
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

            MongoDB::instance()
                .set_db_and_collection(STRATEGY_DB_NAME, "websocket_spot_issue")
                .insert_one({
                    {"type", "disconnect"},
                    {"time", nanos}
                });

            // Re-start
            spdlog::info("BinanceQuoterSpot - disconnect, re-starting");
            this->init_websocket();

            co_return;
        },
        // on_close
        [this]() -> Task<void>
        {
            spdlog::info("BinanceQuoterSpot close");

            // Save when websocket spot close
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

            MongoDB::instance()
                .set_db_and_collection(STRATEGY_DB_NAME, "websocket_spot_issue")
                .insert_one({
                    {"type", "close"},
                    {"time", nanos}
                });

            co_return;
        }
    );

    m_websocket->connect(m_ws_url, m_ws_port, "/ws/" + m_listen_key);
}

Task<std::string> BinanceQuoterSpot::get_listen_key()
{
    auto client = std::make_shared<HttpsClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), m_url, m_port);
    client->add_header("X-MBX-APIKEY", m_api_key);

    std::string str = co_await client->post("/api/v3/userDataStream", "");
    Json data = Json::parse(str);

    if (data.has_field("code") && (double)data["code"] < 0)
    {
        spdlog::error("BinanceQuoterSpot - Error fetching listen key: {}", data);
    }
    else
    {
        spdlog::debug("BinanceQuoterSpot - listenKey: {}", data);
    }

    co_return data["listenKey"];
}

Task<void> BinanceQuoterSpot::keep_listen_key()
{
    auto client = std::make_shared<HttpsClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), m_url, m_port);
    client->add_header("X-MBX-APIKEY", m_api_key);

    std::string str = co_await client->put("/api/v3/userDataStream?listenKey=" + m_listen_key, "");
    Json data = Json::parse(str);

    spdlog::debug("BinanceQuoterSpot, re-active m_listen_key = {}", m_listen_key);

    // Send ping
    m_websocket->send_ping();
}

Task<Json> BinanceQuoterSpot::get_open_orders(std::string symbol)
{
    co_return co_await send_binance_request(RequestMethod::GET, "/api/v3/openOrders", "symbol=" + symbol);
}

Task<void> BinanceQuoterSpot::cancel_all(std::string symbol)
{
    co_await send_binance_request(RequestMethod::DELETE, "/api/v3/openOrders", "symbol=" + symbol);

    co_return;
}

Task<Json> BinanceQuoterSpot::cancel(Order order)
{
    // DELETE /api/v3/order?symbol=BTCUSDT&origClientOrderId=my_custom_id_123&timestamp=1743540000000&signature=abcdef
    std::string query_str;

    spdlog::debug("cancel order: {}", order.to_json());

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&origClientOrderId=" + std::to_string(order.order_id);

    co_return co_await send_binance_request(RequestMethod::DELETE, "/api/v3/order", std::move(query_str));
}

Task<Json> BinanceQuoterSpot::place(Order order)
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

    spdlog::debug("query: {}", query_str);

    co_return co_await send_binance_request(RequestMethod::POST, "/api/v3/order", std::move(query_str));
}
