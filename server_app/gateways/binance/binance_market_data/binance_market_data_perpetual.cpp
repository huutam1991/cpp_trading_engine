#include <gateways/binance/binance_market_data/binance_market_data_perpetual.h>
#include <mongo_db/mongo_db.h>
#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>
#include <time/measure_time.h>

#include <app_constants.h>

#include <json/json.h>
#include <json/json_value.h>
#include <json/json_object.h>

BinanceMarketDataPerpetual::BinanceMarketDataPerpetual(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    // Default is GATEWAY
    m_event_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::GATEWAY);
}

BinanceMarketDataPerpetual::~BinanceMarketDataPerpetual()
{
    spdlog::info("~BinanceMarketDataPerpetual");
}

void BinanceMarketDataPerpetual::start()
{
    Task<void> task = init_order_book();
    task.start_running_on(m_event_base);
}

Task<void> BinanceMarketDataPerpetual::init_order_book()
{
    // Remove order books for instruments that are no longer subscribed
    co_await remove_unsubscribed_instruments();

    // Start WebSocket connections
    for (size_t i = 0; i < m_instruments.size(); i++)
    {
        start_websocket(m_instruments[i]);
    }

    static bool start_sync_order_book = false;
    if (start_sync_order_book == false)
    {
        Task<void> task = check_sync_order_book();
        task.start_running_on(m_event_base);
        start_sync_order_book = true;
    }

    co_return;
}

Task<void> BinanceMarketDataPerpetual::remove_unsubscribed_instruments()
{
    std::vector<const Instrument*> removed_instruments;
    for (const auto& [instrument, _] : m_market_data)
    {
        if (std::find(m_instruments.begin(), m_instruments.end(), instrument) == m_instruments.end())
        {
            removed_instruments.push_back(instrument);
        }
    }

    for (const auto& instrument : removed_instruments)
    {
        m_market_data.erase(instrument);
    }

    co_return;
}

Task<void> BinanceMarketDataPerpetual::check_sync_order_book()
{
    // Loop to send REST request to query orderbook (full) at every 5 seconds, if the orderbook is not synced yet
    while (true)
    {
        for (auto& [_, market_data] : m_market_data)
        {
            if (market_data->orderbook->is_not_synced())
            {
                co_await market_data->orderbook->send_request_get_full_order_book();
            }
            else
            {
                // If synced, print order book
                // order_book->print_order_book();
            }
        }

        co_await Timer::sleep_for(2000);
    }
}

void BinanceMarketDataPerpetual::start_websocket(const Instrument* instrument)
{
    if (m_market_data.find(instrument) != m_market_data.end())
    {
        return; // Already started
    }

    auto market_data = std::make_shared<MarketData>();
    market_data->orderbook = std::make_shared<OrderBook>(instrument->exchange_symbol, 10, m_event_base);
    market_data->trade_data = std::make_shared<BinanceTradeData>(instrument->exchange_symbol, m_event_base);
    m_market_data.insert(std::make_pair(instrument, market_data));
}

void BinanceMarketDataPerpetual::update_url_and_port(const std::string& url, const std::string& port)
{
    m_url = url;
    m_port = port;
}

void BinanceMarketDataPerpetual::subscribe_instruments(std::vector<const Instrument*> instruments)
{
    m_instruments = std::move(instruments);
}