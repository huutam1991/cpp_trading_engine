#include "strategy_mm_state_stop.h"
#include <chrono>

StrategyMarketMakerStateStop::StrategyMarketMakerStateStop(VolumeStat& volume_stat) : m_volume_stat{volume_stat}
{}

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
    return m_volume_stat.get_data();
}

Task<void> StrategyMarketMakerStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        // spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);
    }
    else if (std::holds_alternative<TradeUpdate>(data))
    {
        TradeUpdate trade = std::get<TradeUpdate>(data);
        std::string side = trade.is_buy ? "BUY" : "SELL";

        // spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, side: {}, price: {}, quantity: {}",
        //     trade.instrument->symbol, side, trade.price, trade.quantity);

        m_volume_stat.add_trade_volume(trade);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        double bid_price = snapshot->get_best_bid();
        double ask_price = snapshot->get_best_ask();
        double ask_quantity = snapshot->get_best_ask_quantity();
        double bid_quantity = snapshot->get_best_bid_quantity();

        // spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, bid_price: {}, ask_price: {}", snapshot->instrument->symbol, bid_price, ask_price);
        // spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, bid_quantity: {}, ask_quantity: {}", snapshot->instrument->symbol, bid_quantity, ask_quantity);

        OrderBookSnapShotPool::release(snapshot);
    }

    co_return;
}

// Json StrategyMarketMakerStateStop::get_open_orders()
// {
//     return Json::create_array();
// }