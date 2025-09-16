#include "strategy_mm_state_stop.h"
#include <chrono>

StrategyMarketMakerStateStop::StrategyMarketMakerStateStop()
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
    else if (std::holds_alternative<TradeUpdate>(data))
    {
        TradeUpdate trade = std::get<TradeUpdate>(data);
        spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, trade_id: {}, price: {}, quantity: {}", trade.instrument->symbol, trade.trade_id, trade.price, trade.quantity);
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
    }

    co_return;
}

// Json StrategyMarketMakerStateStop::get_open_orders()
// {
//     return Json::create_array();
// }