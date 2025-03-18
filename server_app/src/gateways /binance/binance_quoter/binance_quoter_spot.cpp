#include <external_request/external_request_ssl.h>
#include <timer.h>
#include <coroutine/event_base_manager.h>

#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <request_future.h>
#include <measure_time.h>

BinanceQuoterSpot::BinanceQuoterSpot(const std::string& key) : BinanceQuoter(key)
{
    m_url = m_is_testnet == true ? BINANCE_TESTNET_SPOT_URL : BINANCE_SPOT_URL;
    m_port = m_is_testnet == true ? BINANCE_TESTNET_SPOT_PORT : BINANCE_SPOT_PORT;

    // websocket
    m_ws_url = m_is_testnet == true ? BINANCE_TESTNET_SPOT_WS_URL : BINANCE_SPOT_WS_URL;
    m_ws_port = m_is_testnet == true ? BINANCE_TESTNET_SPOT_WS_PORT : BINANCE_SPOT_WS_PORT;
    init_websocket();
}

BinanceQuoterSpot::~BinanceQuoterSpot()
{
    del_timer_keep_alive_listen_key();
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
    auto task = this->get_listen_key();
    m_listen_key = task.start_running_on(EventBaseManager::instance().get_event_base_by_id(EventBaseID::STRATEGY)).get();

    // Set period time to re-active m_listen_key at every 30 minutes (1800 seconds)
    add_timer_keep_alive_listen_key(1800000);

    m_websocket = std::make_shared<WebsocketClient>(m_ws_url, m_ws_port, "/ws/" + m_listen_key);

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("BinanceQuoterSpot websocket connected");
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        Json json = Json::parse(buffer);

        if (json["e"] == "executionReport")
        {
            Order order
            {
                1,                                   // Order Id
                Order::ExchangeType::SPOT,           // Exchange Type
                Order::Status::NEW,                  // Status
                json["s"],                           // Symbol
                Order::side_from_string(json["S"]),  // Side
                Order::type_from_string(json["o"]),  // Type
                std::stod((std::string)json["p"]),   // Price
                std::stod((std::string)json["q"]),   // Quantity
            };

            // Parsing order from execution report
            if (json["X"] == "NEW")
            {
                order.order_id = std::stoull((std::string)json["c"]);
                order.status = Order::Status::NEW;
            }
            else if (json["X"] == "FILLED")
            {
                order.order_id = std::stoull((std::string)json["c"]);
                order.status = Order::Status::FILLED;
                order.filled_quantity = std::stod((std::string)json["l"]);
                order.filled_price = std::stod((std::string)json["L"]);
                order.commission_amount = std::stod((std::string)json["n"]);
                order.commission_asset = (std::string)json["N"];
            }
            else if (json["X"] == "PARTIALLY_FILLED")
            {
                order.order_id = std::stoull((std::string)json["c"]);
                order.status = Order::Status::PARTIALLY_FILLED;
                order.filled_quantity = std::stod((std::string)json["l"]);
                order.filled_price = std::stod((std::string)json["L"]);
                order.commission_amount = std::stod((std::string)json["n"]);
                order.commission_asset = (std::string)json["N"];
            }
            else if (json["X"] == "CANCELED")
            {
                order.order_id = std::stoull((std::string)json["C"]);
                order.status = Order::Status::CANCELED;
            }

            // ADD_LOG("BinanceQuoterSpot Order: " << order.to_json());
            OrderManager::instance().update_order(order);
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("BinanceQuoterSpot on_close, close_code = " << close_code);

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

Task<std::string> BinanceQuoterSpot::get_listen_key()
{
    RequestFuture binance_request(m_url, m_port, "/api/v3/userDataStream", RequestMethod::POST);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    Json data = co_await binance_request.send_request();
    co_return data["listenKey"];
}

void BinanceQuoterSpot::add_timer_keep_alive_listen_key(size_t period)
{
    m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    {
        ExternalRequestSsl binance_request(m_url, "443", "/api/v3/userDataStream?listenKey=" + m_listen_key, RequestMethod::PUT);
        binance_request.add_header("X-MBX-APIKEY", m_api_key);

        ADD_LOG("BinanceQuoterSpot re-active m_listen_key = " << m_listen_key);

        std::string res = binance_request.send_request();
        Json data = Json::parse(res);
    },
    period);
}

void BinanceQuoterSpot::del_timer_keep_alive_listen_key()
{
    if (m_schedule_task_id != 0)
    {
        Timer::instance().delete_schedule_task(this->m_schedule_task_id);
    }
}

Json BinanceQuoterSpot::get_trade_result_from_response(Json& response)
{
    // Return empty data if has error
    if ((long)response["code"] < 0)
    {
        ADD_LOG("Spot order error: " << response);
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
            ADD_LOG("fill: " << fill);
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

        ADD_LOG("Spot order place - symbol: " << symbol << ", quantity: " << quantity << ", volumn_in_usdt: " << volumn_in_usdt);
    }

    return {
        {"type", "spot"},
        {"symbol", symbol},
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt}
    };
}

Task<Json> BinanceQuoterSpot::place(Order order)
{
    m_order = order;

    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;

    query_str += "symbol=" + order.symbol;
    query_str += "&side=" + Order::to_string(order.side);
    query_str += "&type=" + Order::to_string(order.type);
    query_str += "&quantity=" + std::to_string(order.quantity);
    query_str += "&newClientOrderId=" + std::to_string(order.order_id);

    if (order.type == Order::OrderType::LIMIT)
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    ADD_LOG("query: " << query_str);

    co_return co_await send_binance_request(RequestMethod::POST, "/api/v3/order", query_str);
}
