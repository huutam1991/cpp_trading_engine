#include <order_book/order_book.h>
#include <utils/dedupe_checker.h>
#include <iomanip>

Orderbook::Orderbook(const std::string& symbol, EventBase* event_base)
    :   m_symbol{symbol},
        m_event_base{event_base}
{}

Task<void> Orderbook::apply_snapshot(Json& snapshot)
{
    // Update Ask
    m_asks.clear();
    snapshot["asks"].for_each([this](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);
        m_asks.insert(std::make_pair(price, quantity));
    });

    // Update Bids
    m_bids.clear();
    snapshot["bids"].for_each([this](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);
        m_bids.insert(std::make_pair(price, quantity));
    });

    // Print logs
    print_order_book();

    co_return;
}

Task<void> Orderbook::apply_book_levels(Json& levels)
{
    // Apply asks
    levels["a"].for_each([this](Json& level)
    {
        // MeasureTime t("Orderbook::OnOrderbookWs, handle level a", MeasureUnit::MICROSECOND);
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
    levels["b"].for_each([this](Json& level)
    {
        // MeasureTime t("Orderbook::OnOrderbookWs, handle level b", MeasureUnit::MICROSECOND);
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

    co_return;
}

void Orderbook::print_order_book()
{
    spdlog::debug("[Rest] Orderbook update snapshot for symbol: {}", m_symbol);
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