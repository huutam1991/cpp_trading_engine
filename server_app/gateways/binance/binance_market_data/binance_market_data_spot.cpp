#include <gateways/binance/binance_market_data/binance_market_data_spot.h>
#include <mongo_db/mongo_db.h>
#include <coroutine/event_base_manager.h>
#include <time/measure_time.h>
#include <strategy/strategy_manager.h>

#include <app_constants.h>

BinanceMarketDataSpot::BinanceMarketDataSpot(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    // Default is GATEWAY
    m_event_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::EPOLL_GATEWAY);
}

BinanceMarketDataSpot::~BinanceMarketDataSpot()
{
    // del_timer_to_check_websocket_stream_is_stop();
    spdlog::debug("~BinanceMarketDataSpot");
}

void BinanceMarketDataSpot::start()
{
    // Close all remaining websockets
    m_websockets.clear();

    for (size_t i = 0; i < m_instruments.size(); i++)
    {
        start_websocket(m_instruments[i]);
    }
}

void BinanceMarketDataSpot::start_websocket(const Instrument* instrument)
{
    if (m_websockets.find(instrument) != m_websockets.end())
    {
        return; // Already started
    }

    auto websocket = std::make_shared<HttpsClientWebsocket>(m_event_base, m_url, std::stoi(m_port), "/ws",
        // On connect
        [this, instrument]() -> Task<void>
        {
            spdlog::info("Binance websocket depth connected");

            // Subcribe for depth
            size_t stream_id = get_stream_id_count();
            std::string lower_case_symbol = instrument->exchange_symbol;
            STRING_LOWER_CASE(lower_case_symbol);

            Json params;
            params[0] = lower_case_symbol + "@depth5@1000ms";

            Json subcribe;
            subcribe["method"] = "SUBSCRIBE";
            subcribe["params"] = params;
            subcribe["id"] = stream_id;

            spdlog::info("subcribe = {}", subcribe);

            co_return;
        },
        // on_message
        [this, instrument](std::string buffer) -> Task<void>
        {
            // MeasureTime t("Handle price update", MeasureUnit::MICROSECOND);

            Json depth = Json();
            if (this->standardize_data(buffer, depth))
            {
                double best_bid = depth["bids"][0][0];
                double best_ask = depth["asks"][0][0];
                double mid = (best_bid + best_ask) / 2.0;

                StrategyUpdateData price{PriceUpdate{instrument, mid}};
                StrategyManager::instance().public_data(price);
            }
            else
            {
                // // Save this none json data for checking error
                // MongoDB::instance()
                //     .set_db_and_collection(STRATEGY_DB_NAME, "websocket_invalid_market_data")
                //     .insert_one(Json::parse(buffer));
            }

            co_return;
        },
        // on_disconnect
        [this, instrument]() -> Task<void>
        {
            // Re-start
            spdlog::debug("Disconnect, re-start BinanceMarketDataSpot");

            co_return;
        },
        // on_close
        []() -> Task<void>
        {
            spdlog::debug("BinanceMarketDataSpot close");
            co_return;
        }
    );

    m_websockets.insert(std::make_pair(instrument, websocket));
}

size_t BinanceMarketDataSpot::get_stream_id_count()
{
    static int stream_id_count = 0;
    return ++stream_id_count;
}

void BinanceMarketDataSpot::update_url_and_port(const std::string& url, const std::string& port)
{
    m_url = url;
    m_port = port;
}

void BinanceMarketDataSpot::subscribe_instruments(std::vector<const Instrument*> instruments)
{
    m_instruments = std::move(instruments);
}

bool BinanceMarketDataSpot::standardize_data(const std::string& data, Json& depth)
{
    MeasureTime t("Standardize data SPOT", MeasureUnit::MICROSECOND);
    Json order_book = Json::parse(data);

    // spdlog::debug(order_book);

    if (order_book.has_field("asks") && order_book.has_field("bids"))
    {
        // symbol
        depth["s"] = "m_symbol";
        // event name
        depth["e"] = "depthUpdate";

        // update asks
        Json A;
        Json asks = order_book["asks"];
        asks.for_each([&A](Json& data)
        {
            Json j;
            j.push_back(std::stold((std::string&&)data[0]));
            j.push_back(std::stold((std::string&&)data[1]));
            A.push_back(j);
        });
        depth["asks"] = A;

        // update bids
        Json B;
        Json bids = order_book["bids"];
        bids.for_each([&B](Json& data)
        {
            Json j;
            j.push_back(std::stold((std::string&&)data[0]));
            j.push_back(std::stold((std::string&&)data[1]));
            B.push_back(j);
        });
        depth["bids"] = B;

        return true;
    }

    return false;
}