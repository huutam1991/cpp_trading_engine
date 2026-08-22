#include <network/external_request/https_client_async.h>
#include <time/timer.h>
#include <time/measure_time.h>

#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <network/https_client_request/https_client_request.h>
#include <app_utils/app_utils.h>

BinanceQuoterSpot::BinanceQuoterSpot(std::shared_ptr<AccountBase> account) : BinanceQuoter(account)
{
    if (account->get_exchange_id() == ExchangeId::BINANCE_TESTNET)
    {
        m_url = BINANCE_TESTNET_SPOT_REST_URL;
        m_port = BINANCE_TESTNET_SPOT_REST_PORT;
        m_ws_url = BINANCE_TESTNET_SPOT_WS_URL;
        m_ws_port = BINANCE_TESTNET_SPOT_WS_PORT;
    }
    else
    {
        m_url = BINANCE_SPOT_REST_URL;
        m_port = BINANCE_SPOT_REST_PORT;
        m_ws_url = BINANCE_SPOT_WS_URL;
        m_ws_port = BINANCE_SPOT_WS_PORT;
    }

    // Event base: GATEWAY
    m_epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY);

    init_websocket();

    m_keep_listen_key_task = keep_listen_key();
    m_keep_listen_key_task.start_running_on(m_epoll_base);
}

BinanceQuoterSpot::~BinanceQuoterSpot()
{
    m_keep_listen_key_task.destroy();
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
    // Get listen key
    auto task = this->get_listen_key();
    auto future = task.get_future();

    task.start_running_on(m_epoll_base);
    m_listen_key = future.get();

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_epoll_base, m_ws_url, std::stoi(m_ws_port), "/ws",
        // on_connect
        [this]() -> Task<void>
        {
            spdlog::info("BinanceQuoterSpot websocket connected");
            co_return;
        },
        // on_message
        [this](std::string buffer) -> Task<void>
        {
            // MeasureTime a("Handle order data");
            Json json = Json::parse(buffer);

            spdlog::warn("BinanceQuoterSpot websocket message: {}", json);

            if (json["e"] == "executionReport")
            {
                std::string exchange_symbol = json["s"];
                const Instrument* instrument = Instrument::get_instrument_by_exchange_symbol(m_exchange_id, InstrumentType::SPOT, exchange_symbol);

                Order order
                {
                    0,                                   // Order Id
                    Order::Status::NEW,                  // Status
                    instrument,                          // Instrument
                    enum_reflect::enum_value<Order::Side>((std::string)json["S"]), // Side
                    enum_reflect::enum_value<Order::OrderType>((std::string)json["o"]), // Type
                    std::stod((std::string)json["p"]),   // Price
                    std::stod((std::string)json["q"]),   // Quantity
                };

                // Parsing client order id and source
                std::string client_order_id = json["c"];
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

                // Parsing order from execution report
                if (json["X"] == "NEW")
                {
                    order.status = Order::Status::NEW;
                    order.last_updated = (size_t)json["T"] * 1000000;
                }
                else if (json["X"] == "FILLED")
                {
                    order.status = Order::Status::FILLED;
                    order.last_updated = (size_t)json["T"] * 1000000;
                    order.filled_quantity = std::stod((std::string)json["l"]);
                    order.filled_price = std::stod((std::string)json["L"]);
                    order.fee = std::stod((std::string)json["n"]);
                    order.commission_asset = (std::string)json["N"];
                }
                else if (json["X"] == "PARTIALLY_FILLED")
                {
                    order.status = Order::Status::PARTIALLY_FILLED;
                    order.last_updated = (size_t)json["T"] * 1000000;
                    order.filled_quantity = std::stod((std::string)json["l"]);
                    order.filled_price = std::stod((std::string)json["L"]);
                    order.fee = std::stod((std::string)json["n"]);
                    order.commission_asset = (std::string)json["N"];
                }
                else if (json["X"] == "CANCELED")
                {
                    order.status = Order::Status::CANCELED;
                    order.last_updated = (size_t)json["T"] * 1000000;
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
            spdlog::info("BinanceQuoterSpot - disconnect, re-starting");

            co_return;
        },
        // on_close
        [this]() -> Task<void>
        {
            spdlog::info("BinanceQuoterSpot close");

            co_return;
        }
    );
}

Task<std::string> BinanceQuoterSpot::get_listen_key()
{
    HttpsClientRequest client(m_epoll_base, m_url, std::stoi(m_port));
    client.add_header("X-MBX-APIKEY", m_api_key);

    HttpsClientResponse response = co_await client.post("/api/v3/userDataStream", "");
    Json data = Json::parse(response.body);

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
    while (true)
    {
        co_await Timer::sleep_for(30000);

        HttpsClientRequest client(m_epoll_base, m_url, std::stoi(m_port));
        client.add_header("X-MBX-APIKEY", m_api_key);

        HttpsClientResponse response = co_await client.put("/api/v3/userDataStream?listenKey=" + m_listen_key, "");
        Json data = Json::parse(response.body);

        spdlog::debug("BinanceQuoterSpot, re-active m_listen_key = {}", m_listen_key);
    }
}

Task<Json> BinanceQuoterSpot::get_open_orders(std::string symbol)
{
    HttpsClientRequest client(m_epoll_base, get_url(), std::stoi(get_port()));
    client.add_header("X-MBX-APIKEY", m_api_key);

    co_return co_await send_binance_request(RequestMethod::GET, "/api/v3/openOrders", "symbol=" + symbol, &client);
}

Task<void> BinanceQuoterSpot::cancel_all(std::string symbol)
{
    HttpsClientRequest client(m_epoll_base, get_url(), std::stoi(get_port()));
    client.add_header("X-MBX-APIKEY", m_api_key);

    co_await send_binance_request(RequestMethod::DELETE, "/api/v3/openOrders", "symbol=" + symbol, &client);

    co_return;
}

Task<Json> BinanceQuoterSpot::cancel(Order order)
{
    // DELETE /api/v3/order?symbol=BTCUSDT&origClientOrderId=my_custom_id_123&timestamp=1743540000000&signature=abcdef
    std::string query_str;

    spdlog::debug("cancel order: {}", order.to_json());

    query_str += "symbol=" + order.instrument->exchange_symbol.to_string();
    query_str += "&origClientOrderId=" + std::to_string(order.order_id);

    HttpsClientRequest client(m_epoll_base, get_url(), std::stoi(get_port()));
    client.add_header("X-MBX-APIKEY", m_api_key);

    co_return co_await send_binance_request(RequestMethod::DELETE, "/api/v3/order", std::move(query_str), &client);
}

Task<Json> BinanceQuoterSpot::place(Order order)
{
    PipelineTraceBuffer::RecordStageTiming<PipelineStage::SEND_ORDER> record_stage_timing(order.trace_id, true);

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

    HttpsClientRequest client(m_epoll_base, get_url(), std::stoi(get_port()));
    client.add_header("X-MBX-APIKEY", m_api_key);

    co_return co_await send_binance_request(RequestMethod::POST, "/api/v3/order", std::move(query_str), &client);
}
