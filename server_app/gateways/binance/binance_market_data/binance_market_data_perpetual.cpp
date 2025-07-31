#include <gateways/binance/binance_market_data/binance_market_data_perpetual.h>
#include <mongo_db/mongo_db.h>
#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>
#include <time/measure_time.h>

#include <app_constants.h>

#include <c_json/json.h>
#include <c_json/json_value.h>
#include <c_json/json_object.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

BinanceMarketDataPerpetual::BinanceMarketDataPerpetual(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    // Default is GATEWAY
    m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY);
}

BinanceMarketDataPerpetual::~BinanceMarketDataPerpetual()
{
    spdlog::info("~BinanceMarketDataPerpetual");
}

void BinanceMarketDataPerpetual::start()
{
    // Close all remaining websockets
    for (auto& [_, websocket] : m_websockets)
    {
        websocket->close();
    }
    m_websockets.clear();

    for (size_t i = 0; i < m_instruments.size(); i++)
    {
        start_websocket(m_instruments[i]);
    }
}

void BinanceMarketDataPerpetual::start_websocket(const Instrument* instrument)
{
    if (m_websockets.find(instrument) != m_websockets.end())
    {
        return; // Already started
    }

    auto websocket = std::make_shared<WebsocketClientAsync>(IOCPool::get_ioc_by_id(IOCId::MARKET_DATA), m_event_base);
    m_websockets.insert(std::make_pair(instrument, websocket));

    websocket->set_callbacks(
        // on_connect
        [this, instrument, websocket = std::weak_ptr<WebsocketClientAsync>(websocket)]() -> TaskVoid
        {
            spdlog::info("Binance websocket depth connected");

            // Subcribe for depth
            size_t stream_id = get_stream_id_count();
            std::string lower_case_symbol = instrument->exchange_symbol;
            STRING_LOWER_CASE(lower_case_symbol);

            JsonNew params;
            params[0] = lower_case_symbol + "@depth5@500ms";

            JsonNew subcribe;
            subcribe["method"] = "SUBSCRIBE";
            subcribe["params"] = params;
            subcribe["id"] = stream_id;

            spdlog::info("subcribe = {}", subcribe);

            if (auto ws = websocket.lock())
            {
                ws->send(subcribe.get_string_value());

                // Set period time to send ping frame at every 30 seconds
                ws->add_keep_websocket_alive_task([this, websocket = std::weak_ptr<WebsocketClientAsync>(ws)]() -> TaskVoid
                {
                    if (auto ws = websocket.lock())
                    {
                        ws->send_ping();
                    }

                    co_return;
                }, CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD);
            }

            co_return;
        },
        // on_message
        [this, instrument](std::string buffer) -> TaskVoid
        {
            // MeasureTime t("Handle price update PERPETUAL", MeasureUnit::MICROSECOND);

            JsonNew depth = JsonNew();
            if (this->standardize_data(std::move(buffer), depth))
            {
                // spdlog::debug("Stream depth: {}", depth);
                // if (m_on_callback != nullptr)
                // {
                //     m_on_callback(instrument, depth);
                // }
            }
            else
            {
                // // Save this none json data for checking error
                // MongoDB::instance()
                //     .set_db_and_collection(STRATEGY_DB_NAME, "websocket_invalid_market_data")
                //     .insert_one(JsonNew::parse(buffer));
            }

            co_return;
        },
        // on_disconnect
        [this, instrument]() -> TaskVoid
        {
            // Re-start
            spdlog::debug("Disconnect, re-start BinanceMarketDataPerpetual");
            this->start_websocket(instrument);

            co_return;
        },
        // on_close
        []() -> TaskVoid
        {
            spdlog::debug("BinanceMarketDataPerpetual close");
            co_return;
        }
    );

    websocket->connect(m_url, m_port, "/ws");
}

size_t BinanceMarketDataPerpetual::get_stream_id_count()
{
    static int stream_id_count = 0;
    return ++stream_id_count;
}

void BinanceMarketDataPerpetual::update_url_and_port(const std::string& url, const std::string& port)
{
    m_url = url;
    m_port = port;
}

void BinanceMarketDataPerpetual::subscribe_instruments(std::vector<const Instrument*> instruments, std::function<void(const Instrument* symbol, JsonNew& payload)> call_back)
{
    m_instruments = std::move(instruments);
    m_on_callback = std::move(call_back);
}

bool BinanceMarketDataPerpetual::standardize_data(std::string data, JsonNew& depth)
{
    MeasureTime t("Standardize data PERPETUAL", MeasureUnit::MICROSECOND);
    JsonNew order_book;
    {
        MeasureTime("Parse order book PERPETUAL", MeasureUnit::MICROSECOND),
        order_book = JsonNew::parse(std::move(data));
    }
    // spdlog::debug("BinanceMarketDataPerpetual - orderbook: {}", order_book);

    JsonNew depth_new;

    if (order_book.has_field("a") && order_book.has_field("b"))
    {
        MeasureTime t2("build depth PERPETUAL", MeasureUnit::MICROSECOND);
        // symbol
        depth_new["s"] = "m_symbol";
        // event name
        depth_new["e"] = "depthUpdate";

        // update asks
        JsonNew A;
        JsonNew asks = order_book["a"];
        A.set_size(asks.size());
        asks.for_each([&A](JsonNew& data)
        {
            JsonNew j;
            JsonNew a0 = std::stod((std::string)data[0]);
            JsonNew a1 = std::stod((std::string)data[1]);
            j.push_back(a0);
            j.push_back(a1);
            A.push_back(j);
        });
        depth_new["asks"] = A;

        // update bids
        JsonNew B;
        JsonNew bids = order_book["b"];
        B.set_size(bids.size());
        bids.for_each([&B](JsonNew& data)
        {
            JsonNew j;
            JsonNew b0 = std::stod((std::string)data[0]);
            JsonNew b1 = std::stod((std::string)data[1]);
            j.push_back(b0);
            j.push_back(b1);
            B.push_back(j);
        });
        depth_new["bids"] = B;

        return true;
    }

    return false;
}