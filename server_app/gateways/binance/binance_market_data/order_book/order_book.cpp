#include <gateways/binance/binance_market_data/order_book/order_book.h>
#include <utils/dedupe_checker.h>
#include <iomanip>

OrderBook::OrderBook(const std::string& symbol, size_t depth_level, net::io_context& ioc, EventBase* event_base)
    : m_symbol{symbol},
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

TaskVoid OrderBook::send_request_get_full_order_book()
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
    if (DedupeChecker::is_duplicate(data) == true)
    {
        std::cout << "[WS] data is duplicate: " << data << std::endl;
        return;
    }

    Json update = Json::parse(data);

    uint64_t pu = update["pu"];
    uint64_t u  = update["u"];
    uint64_t U  = update["U"];

    if (!m_snapshot_loaded)
    {
        std::cout << "[WS] symbol: [" << m_symbol << "], Snapshot not loaded — skipping update\n";
        return;
    }

    if (m_ws_waiting_first_event)
    {
        if (U <= m_ws_last_update_id && u >= m_ws_last_update_id)
        {
            m_ws_waiting_first_event = false;
            m_ws_last_update_id = u; // Sync from here
            std::cout << "[WS] symbol: [" << m_symbol << "], First valid event applied: U=" << U << ", u=" << u << "\n";
        }
        else
        {
            std::cout << "[WS] symbol: [" << m_symbol << "], Waiting for first valid event. Got U=" << U << ", u=" << u
                      << ", expected to cover lastUpdateId=" << m_ws_last_update_id << "\n";
            return;
        }
    }
    else
    {
        // Only after sync is started, we enforce pu == lastUpdateId
        if (pu != m_ws_last_update_id)
        {
            std::cout << "[WS] symbol: [" << m_symbol << "], Update chain broken: pu=" << pu << ", expected=" << m_ws_last_update_id
                      << " -> triggering snapshot reload\n";
            m_snapshot_loaded = false;
            m_ws_waiting_first_event = true;
            return;
        }

        m_ws_last_update_id = u;
    }

    // Apply asks
    update["a"].for_each([this](Json& level)
    {
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

    std::cout << "[WS] symbol: [" << m_symbol << "], Update applied successfully: u=" << u << "\n";
}

void OrderBook::OnOrderbookRest(std::string data)
{
    if (DedupeChecker::is_duplicate(data) == true)
    {
        std::cout << "[Rest] data is duplicate: " << data << std::endl;
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

void OrderBook::print_order_book()
{
    std::cout << "[Rest] OrderBook update snapshot for symbol: " << m_symbol << std::endl;
    std::cout << "[Rest] asks: " << std::endl;
    for (auto& [price, quantity] : m_asks)
    {
        std::cout << std::setprecision(15) << "[Rest] [" << price << " - " << quantity << "], " << std::endl;
    }

    std::cout << "[Rest] bids: " << std::endl;
    for (auto& [price, quantity] : m_bids)
    {
        std::cout << std::setprecision(15) << "[Rest] [" << price << " - " << quantity << "], " << std::endl;
    }
    std::cout << std::endl;
}