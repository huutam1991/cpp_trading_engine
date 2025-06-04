#include <gateways/binance/binance_market_data/binance_market_data.h>
#include <mongo_db/mongo_db.h>
#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>
#include <time/measure_time.h>

#include <app_constants.h>

BinanceMarketData::BinanceMarketData(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    // Default is GATEWAY
    m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY);
}

BinanceMarketData::~BinanceMarketData()
{
    // del_timer_to_check_websocket_stream_is_stop();
    ADD_LOG("~BinanceMarketData");
}

void BinanceMarketData::start()
{
    for (size_t i = 0; i < m_symbols.size(); i++)
    {
        start_websocket(m_symbols[i]);
    }
}

void BinanceMarketData::start_websocket(std::string symbol)
{
    if (m_websockets.find(symbol) != m_websockets.end())
    {
        m_websockets[symbol]->close();
        m_websockets.erase(symbol);
    }

    auto websocket = std::make_shared<WebsocketClientAsync>(IOCPool::get_ioc_by_id(IOCId::MARKET_DATA), m_event_base);
    m_websockets.insert(std::make_pair(symbol, websocket));

    websocket->set_callbacks(
        // on_connect
        [this, symbol, websocket]() -> TaskVoid
        {
            spdlog::info("Binance websocket depth connected");

            // Subcribe for depth
            size_t stream_id = get_stream_id_count();
            std::string lower_case_symbol = symbol;
            STRING_LOWER_CASE(lower_case_symbol);

            Json params;
            params[0] = lower_case_symbol + "@depth5@1000ms";

            Json subcribe;
            subcribe["method"] = "SUBSCRIBE";
            subcribe["params"] = params;
            subcribe["id"] = stream_id;

            spdlog::info("subcribe = {}", subcribe);

            websocket->send(subcribe.get_string_value());

            co_return;
        },
        // on_message
        [this, symbol](std::string buffer) -> TaskVoid
        {
            // MeasureTime t("Handle price update", MeasureUnit::MICROSECOND);

            Json depth = Json();
            if (this->standardize_data(buffer, depth))
            {
                // ADD_LOG("Stream depth: " << depth);
                if (m_on_callback != nullptr)
                {
                    m_on_callback(symbol, depth);
                }
            }
            else
            {
                // Save this none json data for checking error
                MongoDB::instance()
                    .set_db_and_collection(STRATEGY_DB_NAME, "websocket_invalid_market_data")
                    .insert_one(Json::parse(buffer));
            }

            co_return;
        },
        // on_disconnect
        [this, symbol]() -> TaskVoid
        {
            // Re-start
            spdlog::info("Disconnect, re-start BinanceMarketData");
            this->start_websocket(symbol);

            co_return;
        },
        // on_close
        []() -> TaskVoid
        {
            spdlog::info("BinanceMarketData close");
            co_return;
        }
    );

    websocket->connect(m_url, m_port, "/ws");
}

size_t BinanceMarketData::get_stream_id_count()
{
    static int stream_id_count = 0;
    return ++stream_id_count;
}

void BinanceMarketData::update_url_and_port(const std::string& url, const std::string& port)
{
    m_url = url;
    m_port = port;
}

void BinanceMarketData::subscribe_symbol(std::vector<std::string> symbols, std::function<void(const std::string& symbol, Json& payload)> call_back)
{
    m_symbols = std::move(symbols);
    m_on_callback = std::move(call_back);
}

bool BinanceMarketData::standardize_data(const std::string& data, Json& depth)
{
    Json order_book = Json::parse(data);

    // ADD_LOG(order_book);

    if (order_book.has_field("asks") && order_book.has_field("bids"))
    {
        // symbol
        depth["s"] = "m_symbol";
        // event name
        depth["e"] = "depthUpdate";

        // update asks
        Json A = Json::create_array();
        Json asks = order_book["asks"];
        asks.for_each([&A](Json& data)
        {
            Json j = Json::create_array();
            j.push_back(std::stold((std::string&&)data[0]));
            j.push_back(std::stold((std::string&&)data[1]));
            A.push_back(j);
        });
        depth["asks"] = A;

        // update bids
        Json B = Json::create_array();
        Json bids = order_book["bids"];
        bids.for_each([&B](Json& data)
        {
            Json j = Json::create_array();
            j.push_back(std::stold((std::string&&)data[0]));
            j.push_back(std::stold((std::string&&)data[1]));
            B.push_back(j);
        });
        depth["bids"] = B;

        return true;
    }

    return false;
}