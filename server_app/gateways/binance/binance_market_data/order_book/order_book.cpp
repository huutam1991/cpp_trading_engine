#include <gateways/binance/binance_market_data/order_book/order_book.h>
#include <utils/dedupe_checker.h>
#include <iomanip>

OrderBook::OrderBook(const std::string& symbol, size_t depth_level, net::io_context& ioc, EventBase* event_base)
    :   m_symbol{symbol},
        m_instrument{Instrument::get_instrument_by_exchange_symbol(ExchangeId::BINANCE, InstrumentType::PERPETUAL, symbol)},
        m_depth_level{depth_level},
        m_order_book_websocket{
            symbol,
            depth_level,
            ioc,
            event_base,
            [this](std::string data) { this->OnOrderbookWs(std::move(data)); }
        },
        m_order_book_rest{ioc}
{}

Task<void> OrderBook::send_request_get_full_order_book()
{
    std::string data = co_await m_order_book_rest.get_order_book(m_symbol, m_depth_level);

    // Update to method OnOrderbookRest
    OnOrderbookRest(data);

    co_return;
}

bool OrderBook::is_not_synced()
{
    return m_snapshot_loaded == false || m_ws_waiting_first_event == true;
}

void OrderBook::OnOrderbookWs(std::string data)
{
    MeasureTime t("OrderBook::OnOrderbookWs [" + m_symbol + "], handle from websocket", MeasureUnit::MICROSECOND);
    if (DedupeChecker::is_duplicate(data) == true)
    {
        spdlog::debug("[WS] data is duplicate: {}", data);
        return;
    }

    Json update = Json::parse(std::move(data));
    // spdlog::debug("[WS] symbol: [{}], update: {}", m_symbol, update);

    uint64_t pu = update["pu"];
    uint64_t u  = update["u"];
    uint64_t U  = update["U"];

    if (!m_snapshot_loaded)
    {
        spdlog::debug("[WS] symbol: [{}], Snapshot not loaded — skipping update", m_symbol);
        return;
    }

    if (m_ws_waiting_first_event)
    {
        if (U <= m_ws_last_update_id && u >= m_ws_last_update_id)
        {
            m_ws_waiting_first_event = false;
            m_ws_last_update_id = u; // Sync from here
            spdlog::debug("[WS] symbol: [{}], First valid event applied: U={}, u={}", m_symbol, U, u);
        }
        else
        {
            spdlog::debug("[WS] symbol: [{}], Waiting for first valid event. Got U={}, u={}, expected to cover lastUpdateId={}", m_symbol, U, u, m_ws_last_update_id);
            return;
        }
    }
    else
    {
        // Only after sync is started, we enforce pu == lastUpdateId
        if (pu != m_ws_last_update_id)
        {
            spdlog::debug("[WS] symbol: [{}], Update chain broken: pu={}, expected={} -> triggering snapshot reload", m_symbol, pu, m_ws_last_update_id);
            m_snapshot_loaded = false;
            m_ws_waiting_first_event = true;
            return;
        }

        m_ws_last_update_id = u;
    }

    m_asks.clear();
    m_bids.clear();

    // Apply asks
    update["a"].for_each([this](Json& level)
    {
        // MeasureTime t("OrderBook::OnOrderbookWs, handle level a", MeasureUnit::MICROSECOND);
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);
        if (quantity == 0.0)
        {
            m_asks.erase(price);
        }
        else
        {
            m_asks[price] = quantity;
        }
    });

    // Apply bids
    update["b"].for_each([this](Json& level)
    {
        // MeasureTime t("OrderBook::OnOrderbookWs, handle level b", MeasureUnit::MICROSECOND);
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);
        if (quantity == 0.0)
        {
            m_bids.erase(price);
        }
        else
        {
            m_bids[price] = quantity;
        }
    });

    // spdlog::debug("[WS] symbol: [{}], Update applied successfully: u={}, m_asks.size()={}, m_bids.size()={}", m_symbol, u, m_asks.size(), m_bids.size());
}

void OrderBook::OnOrderbookRest(std::string data)
{
    if (DedupeChecker::is_duplicate(data) == true)
    {
        spdlog::debug("[Rest] data is duplicate: {}", data);
        return;
    }

    Json snapshot = Json::parse(data);

    uint64_t snapshot_id = snapshot["lastUpdateId"];

    // Check dedupe m_last_update_id
    if (m_snapshot_last_update_id != snapshot_id)
    {
        m_snapshot_last_update_id = snapshot_id;
        apply_snapshot(snapshot);
    }
}

void OrderBook::apply_snapshot(Json& snapshsot)
{
    // Update Ask
    m_asks.clear();
    snapshsot["asks"].for_each([this](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);
        m_asks.insert(std::make_pair(price, quantity));
    });

    // Update Bids
    m_bids.clear();
    snapshsot["bids"].for_each([this](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);
        m_bids.insert(std::make_pair(price, quantity));
    });

    m_snapshot_loaded = true;
    m_ws_waiting_first_event = true;
    m_ws_last_update_id = m_snapshot_last_update_id;

    // Print logs
    print_order_book();
}

void OrderBook::export_snapshot()
{
    OrderBookSnapShot* snapshot = OrderBookSnapShotPool::acquire();
    snapshot->update_instrument(m_instrument);

    for (const auto& [price, quantity] : m_bids)
    {
        snapshot->add_bid(price, quantity);
    }
    for (const auto& [price, quantity] : m_asks)
    {
        snapshot->add_ask(price, quantity);
    }

    OrderBookManager::instance().publish_order_book_snapshot(snapshot);
}

void OrderBook::print_order_book()
{
    spdlog::debug("[Rest] OrderBook update snapshot for symbol: {}", m_symbol);
    spdlog::debug("[Rest] asks: ");
    for (auto& [price, quantity] : m_asks)
    {
        spdlog::debug("[Rest] [{} - {}], ", price, quantity);
    }

    spdlog::debug("[Rest] bids: ");
    for (auto& [price, quantity] : m_bids)
    {
        spdlog::debug("[Rest] [{} - {}], ", price, quantity);
    }
}