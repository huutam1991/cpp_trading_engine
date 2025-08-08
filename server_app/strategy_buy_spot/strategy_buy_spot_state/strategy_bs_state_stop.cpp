#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_stop.h>

StrategyBuySpotStateStop::StrategyBuySpotStateStop()
{
}

void StrategyBuySpotStateStop::begin()
{
    spdlog::info("StrategyBuySpotStateStop - begin");
}

void StrategyBuySpotStateStop::end()
{
    spdlog::info("StrategyBuySpotStateStop - end");
}

Task<void> StrategyBuySpotStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        double mid_price = (snapshot->get_best_bid() + snapshot->get_best_ask()) / 2;
        double ask_quantity = snapshot->get_best_ask_quantity();
        double bid_quantity = snapshot->get_best_bid_quantity();
        spdlog::info("StrategyBuySpotStateStop: do nothing, snapshot for symbol: {}, mid_price: {}", snapshot->instrument->symbol, mid_price);
        spdlog::info("StrategyBuySpotStateStop: do nothing, snapshot for symbol: {}, ask_quantity: {}, bid_quantity: {}", snapshot->instrument->symbol, ask_quantity, bid_quantity);

        // Release the snapshot back to the pool
        OrderBookSnapShotPool::release(snapshot);
    }

    co_return;
}

// Json StrategyBuySpotStateStop::get_open_orders()
// {
//     return Json::create_array();
// }