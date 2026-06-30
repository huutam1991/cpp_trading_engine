#include <coroutine/event_base_manager.h>
#include <network/https_client_request/https_client_request.h>
#include <utils/utils.h>
#include <mongo_db/mongo_db.h>

#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>
#include <app_utils/app_utils.h>

#include <time/measure_time.h>

BinanceQuoterPerpetual::BinanceQuoterPerpetual(std::shared_ptr<AccountBase> account)
    : BinanceQuoter(account), m_epoll_base{(EpollBase*)EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY)}
{
    if (account->get_exchange_id() == ExchangeId::BINANCE_TESTNET)
    {
        m_url = BINANCE_TESTNET_FUTURES_REST_URL;
        m_port = BINANCE_TESTNET_FUTURES_REST_PORT;

        // websocket
        m_ws_url = BINANCE_TESTNET_FUTURES_WS_URL;
        m_ws_port = BINANCE_TESTNET_FUTURES_WS_PORT;
    }
    else
    {
        m_url = BINANCE_FUTURES_REST_URL;
        m_port = BINANCE_FUTURES_REST_PORT;

        // websocket
        m_ws_url = BINANCE_FUTURES_WS_URL;
        m_ws_port = BINANCE_FUTURES_WS_PORT;
    }

    m_client = std::make_shared<HttpsClientRequest>(m_epoll_base, m_url, std::stoi(m_port));
    m_client->add_header("X-MBX-APIKEY", m_api_key);

    init_websocket();

    m_keep_listen_key_task = keep_listen_key();
    m_keep_listen_key_task.start_running_on(m_epoll_base);
}

BinanceQuoterPerpetual::~BinanceQuoterPerpetual()
{
    m_keep_listen_key_task.destroy();
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
        m_websocket = nullptr;
    }

    // Event base: GATEWAY
    EventBase* event_base = EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY);

    auto task = this->get_listen_key();
    auto future = task.get_future();

    task.start_running_on(event_base);
    m_listen_key = future.get();

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_epoll_base, m_ws_url, std::stoi(m_ws_port), "/private/ws/" + m_listen_key,
        // on_connect
        [this]() -> Task<void>
        {
            spdlog::info("BinanceQuoterPerpetual websocket connected");

            MongoDB::instance()
                .set_db_and_collection("websocket_monitoring", "BinanceQuoterPerpetual")
                .insert_one(Json{
                    {"event", "CONNECTED"},
                    {"timestamp", Utils::get_time_now_in_string_HMS_DMY()}
                }
            );

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
                const Instrument* instrument = Instrument::get_instrument_by_exchange_symbol(m_exchange_id, InstrumentType::PERPETUAL, exchange_symbol);

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

                // Parsing client order id and source
                std::string client_order_id = o["c"];
                order.order_id = AppUtils::parse_order_id(client_order_id);

                if (client_order_id.starts_with("web_coin_"))
                {
                    order.source.type = Order::Source::WEB;
                }
                else if (client_order_id.starts_with("ios_coin_"))
                {
                    order.source.type = Order::Source::IOS;
                }
                else if (client_order_id.starts_with("android_coin_"))
                {
                    order.source.type = Order::Source::ANDROID;
                }

                if (o["X"] == "NEW")
                {
                    order.status = Order::Status::NEW;
                    order.last_updated = (size_t)o["T"] * 1000000;
                }
                else if (o["X"] == "FILLED")
                {
                    order.status = Order::Status::FILLED;
                    order.last_updated = (size_t)o["T"] * 1000000;
                    order.filled_quantity = std::stod((std::string)o["l"]);
                    order.filled_price = std::stod((std::string)o["L"]);
                    order.fee = std::stod((std::string)o["n"]);
                    order.commission_asset = (std::string)o["N"];
                }
                else if (o["X"] == "PARTIALLY_FILLED")
                {
                    order.status = Order::Status::PARTIALLY_FILLED;
                    order.last_updated = (size_t)o["T"] * 1000000;
                    order.filled_quantity = std::stod((std::string)o["l"]);
                    order.filled_price = std::stod((std::string)o["L"]);
                    order.fee = std::stod((std::string)o["n"]);
                    order.commission_asset = (std::string)o["N"];
                }
                else if (o["X"] == "CANCELED")
                {
                    order.status = Order::Status::CANCELED;
                    order.last_updated = (size_t)o["T"] * 1000000;
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

            co_return;
        },
        // on_close
        []() -> Task<void>
        {
            spdlog::info("BinanceQuoterPerpetual - close");
            co_return;
        }
    );
}

Task<std::string> BinanceQuoterPerpetual::get_listen_key()
{
    HttpsClientResponse response = co_await m_client->post("/fapi/v1/listenKey", "");
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
    while (true)
    {
        co_await Timer::sleep_for(30000);

        HttpsClientResponse response = co_await m_client->put("/fapi/v1/listenKey?listenKey=" + m_listen_key, "");
        Json data = Json::parse(response.body);

        spdlog::info("BinanceQuoterPerpetual, re-active m_listen_key: {}", data);
    }
}

Task<Json> BinanceQuoterPerpetual::get_open_orders(std::string symbol)
{
    co_return co_await send_binance_request(RequestMethod::GET, "fapi/v1/openOrders", "symbol=" + symbol, m_client.get());
}

Task<void> BinanceQuoterPerpetual::cancel_all(std::string symbol)
{
    co_await send_binance_request(RequestMethod::DELETE, "/fapi/v1/allOpenOrders", "symbol=" + symbol, m_client.get());
    co_return;
}

Task<Json> BinanceQuoterPerpetual::cancel(Order order)
{
    // DELETE /api/v3/order?symbol=BTCUSDT&origClientOrderId=my_custom_id_123&timestamp=1743540000000&signature=abcdef
    std::string query_str;

    spdlog::debug("cancel order: {}", order.to_json());

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&origClientOrderId=" + std::to_string(order.order_id);

    co_return co_await send_binance_request(RequestMethod::DELETE, "/fapi/v1/order", std::move(query_str), m_client.get());
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

    co_return co_await send_binance_request(RequestMethod::POST, "/fapi/v1/order", std::move(query_str), m_client.get());
}