#include "strategy_mm_state_stop.h"
#include <chrono>

StrategyMarketMakerStateStop::StrategyMarketMakerStateStop(VolumeStat& volume_stat, PnL& pnl) : m_volume_stat{volume_stat}, m_pnl{pnl}
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
    return {
        {"pnl", m_pnl.get_data()},
        {"volume_stat", m_volume_stat.get_data()}
    };
}

void StrategyMarketMakerStateStop::handle_price_update(PriceUpdate& price)
{
    m_pnl.update_current_price(price.price);
}

void StrategyMarketMakerStateStop::handle_trade_update(TradeUpdate& trade)
{
    std::string side = trade.is_buy ? "BUY" : "SELL";
    // spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, side: {}, price: {}, quantity: {}",
    //     trade.instrument->symbol, side, trade.price, trade.quantity);

    m_volume_stat.add_trade_volume(trade);
}

void StrategyMarketMakerStateStop::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
}

void StrategyMarketMakerStateStop::handle_order_update(Order& order)
{
    // spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, side: {}, price: {}, quantity: {}, status: {}",
    //     order.instrument->symbol, enum_reflect::enum_name(order.side), order.price, order.quantity, enum_reflect::enum_name(order.status));
}