#include "strategy_mm_state_stop.h"
#include <chrono>

StrategyMarketMakerStateStop::StrategyMarketMakerStateStop() :
    m_db_name{enum_reflect::enum_name(EventBaseID::MARKET_MAKER_STRATEGY)},
    m_order_gap_list{SavableObject<OrderGap>::load_objects_map<size_t>(m_db_name, "order_gap", "time")}
{
}

void StrategyMarketMakerStateStop::begin()
{
    spdlog::info("StrategyMarketMakerStateStop - begin");
}

void StrategyMarketMakerStateStop::end()
{
    spdlog::info("StrategyMarketMakerStateStop - end");
}

Json StrategyMarketMakerStateStop::get_info()
{
    Json gap_list;
    for (const auto& [id, object] : m_order_gap_list)
    {
        gap_list.push_back(object.to_json());
    }

    gap_list.sort([](Json& a, Json& b) -> bool
    {
        return (size_t)a["time"] < (size_t)b["time"];
    });

    return gap_list;
}

Task<void> StrategyMarketMakerStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        double bid_price = snapshot->get_best_bid();
        double ask_price = snapshot->get_best_ask();
        double ask_quantity = snapshot->get_best_ask_quantity();
        double bid_quantity = snapshot->get_best_bid_quantity();

        spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, bid_price: {}, ask_price: {}", snapshot->instrument->symbol, bid_price, ask_price);
        spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, bid_quantity: {}, ask_quantity: {}", snapshot->instrument->symbol, bid_quantity, ask_quantity);

        OrderGap gap;
        gap.bid = bid_price;
        gap.ask = ask_price;
        gap.gap = ask_price - bid_price;
        gap.bid_quantity = bid_quantity;
        gap.ask_quantity = ask_quantity;
        gap.time = OrderManager::instance().generate_order_id(); // Order Id is current time in nanoseconds

        SavableObject<OrderGap> object(m_db_name, "order_gap", gap);
        m_order_gap_list.insert(std::make_pair(gap.time, object));

        // Release the snapshot back to the pool
        OrderBookSnapShotPool::release(snapshot);
    }
    co_return;
}

// Json StrategyMarketMakerStateStop::get_open_orders()
// {
//     return Json::create_array();
// }