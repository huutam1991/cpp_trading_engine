#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

StrategyMeanReversionStateStop::StrategyMeanReversionStateStop(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config)
    : StrategyMeanReversionState(gateway, config)
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

TaskVoid StrategyMeanReversionStateStop::run(StrategyMeanReversionData data)
{
    MRPriceUpdate price_update;
    if (std::holds_alternative<MRPriceUpdate>(data))
    {
        price_update = std::get<MRPriceUpdate>(data);
    }

    spdlog::debug("StrategyMeanReversionStateStop: do nothing, symbol: {}, price: {}", price_update.symbol, price_update.price);

    co_return;
}

JsonNew StrategyMeanReversionStateStop::get_open_orders()
{
    return {};
}