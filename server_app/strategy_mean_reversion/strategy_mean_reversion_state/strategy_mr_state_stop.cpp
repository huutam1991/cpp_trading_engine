#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

StrategyMeanReversionStateStop::StrategyMeanReversionStateStop(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyMeanReversionStateStop::begin()
{
    spdlog::info("StrategyMeanReversionStateStop - begin");
}

void StrategyMeanReversionStateStop::end()
{
    spdlog::info("StrategyMeanReversionStateStop - end");
}

Json StrategyMeanReversionStateStop::get_info()
{
    return m_config.to_json();
}

Task<void> StrategyMeanReversionStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        spdlog::debug("StrategyMeanReversionStateStop: do nothing, symbol: {}, price: {}", price_update.instrument->symbol, price_update.price);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        double bid_price = snapshot->get_best_bid();
        double ask_price = snapshot->get_best_ask();
        double ask_quantity = snapshot->get_best_ask_quantity();
        double bid_quantity = snapshot->get_best_bid_quantity();
        // m_pnl.update_current_price((bid_price + ask_price) / 2.0);

        OrderBookSnapShotPool::release(snapshot);
    }

    co_return;
}