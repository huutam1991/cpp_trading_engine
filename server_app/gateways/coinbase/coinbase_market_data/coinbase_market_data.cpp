#include <gateways/coinbase/coinbase_market_data/coinbase_market_data.h>
#include <mongo_db/mongo_db.h>
#include <coroutine/event_base_manager.h>

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