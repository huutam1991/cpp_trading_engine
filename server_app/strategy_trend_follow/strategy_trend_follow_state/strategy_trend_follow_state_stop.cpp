#include "strategy_trend_follow_state_stop.h"
#include <chrono>

StrategyTrendFollowStateStop::StrategyTrendFollowStateStop()
{}

void StrategyTrendFollowStateStop::begin()
{
    spdlog::info("StrategyTrendFollowStateStop - begin");
}

void StrategyTrendFollowStateStop::end()
{
    spdlog::info("StrategyTrendFollowStateStop - end");
}

Json StrategyTrendFollowStateStop::get_info()
{
    return {};
}

Task<void> StrategyTrendFollowStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        spdlog::info("StrategyTrendFollowStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        double bid_price = snapshot->get_best_bid();
        double ask_price = snapshot->get_best_ask();
        double ask_quantity = snapshot->get_best_ask_quantity();
        double bid_quantity = snapshot->get_best_bid_quantity();

        spdlog::info("StrategyTrendFollowStateStop: do nothing, symbol: {}, bid_price: {}, ask_price: {}", snapshot->instrument->symbol, bid_price, ask_price);
        spdlog::info("StrategyTrendFollowStateStop: do nothing, symbol: {}, bid_quantity: {}, ask_quantity: {}", snapshot->instrument->symbol, bid_quantity, ask_quantity);

        // Release the snapshot back to the pool
        OrderBookSnapShotPool::release(snapshot);
    }
    co_return;
}

// Json StrategyTrendFollowStateStop::get_open_orders()
// {
//     return Json::create_array();
// }