#include <gateways/coinbase/coinbase_market_data/coinbase_market_data.h>
#include <mongo_db/mongo_db.h>
#include <coroutine/event_base_manager.h>
#include <ioc_pool.h>

#include <app_constants.h>

CoinbaseMarketData::CoinbaseMarketData(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    // Default is GATEWAY
    m_event_base = EventBaseManager::get_event_base_by_id(EpollBaseID::GATEWAY);
}

CoinbaseMarketData::~CoinbaseMarketData()
{
}

void CoinbaseMarketData::start()
{
    for (size_t i = 0; i < m_instruments.size(); i++)
    {
        start_websocket(m_instruments[i]);
    }
}

void CoinbaseMarketData::start_websocket(const Instrument* instrument)
{
    if (m_websockets.find(instrument) != m_websockets.end())
    {
        m_websockets[instrument]->close();
        m_websockets.erase(instrument);
    }

    auto websocket = std::make_shared<WebsocketClientAsync>(IOCPool::get_ioc_by_id(IOCId::MARKET_DATA), m_event_base);
    m_websockets.insert(std::make_pair(instrument, websocket));

    websocket->set_callbacks(
        // on_connect
        [this, instrument, websocket]() -> Task<void>
        {
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

            websocket->send(subcribe.get_string_value());

            co_return;
        },
        // on_message
        [this, instrument](std::string buffer) -> Task<void>
        {
            Json depth = Json();
            if (this->standardize_data(buffer, depth))
            {
                if (m_on_callback != nullptr)
                {
                    m_on_callback(instrument, depth);
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
        [this, instrument]() -> Task<void>
        {
            // Re-start
            this->start_websocket(instrument);

            co_return;
        },
        // on_close
        []() -> Task<void>
        {
            co_return;
        }
    );

    websocket->connect(m_url, m_port, "/ws");
}

size_t CoinbaseMarketData::get_stream_id_count()
{
    static int stream_id_count = 0;
    return ++stream_id_count;
}

void CoinbaseMarketData::update_url_and_port(const std::string& url, const std::string& port)
{
    m_url = url;
    m_port = port;
}

void CoinbaseMarketData::subscribe_instruments(std::vector<const Instrument*> instruments, std::function<void(const Instrument* symbol, Json& payload)> call_back)
{
    m_instruments = std::move(instruments);
    m_on_callback = std::move(call_back);
}

bool CoinbaseMarketData::standardize_data(const std::string& data, Json& depth)
{
    Json order_book = Json::parse(data);

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