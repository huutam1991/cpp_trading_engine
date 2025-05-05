#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

StrategyMeanReversionStateStop::StrategyMeanReversionStateStop(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config)
    : StrategyMeanReversionState(gateway, config)
{
}

void StrategyMeanReversionStateStop::begin()
{
    ADD_LOG("StrategyMeanReversionStateStop - begin");
}

void StrategyMeanReversionStateStop::end()
{
    ADD_LOG("StrategyMeanReversionStateStop - end");
}

TaskVoid StrategyMeanReversionStateStop::run(StrategyMeanReversionData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
    }

    ADD_LOG("StrategyMeanReversionStateStop - run: Do nothing, symbol: " << price_update.symbol << ", price: " << price_update.price);

    co_return;
}

Json StrategyMeanReversionStateStop::get_open_orders()
{
    return Json::create_array();
}