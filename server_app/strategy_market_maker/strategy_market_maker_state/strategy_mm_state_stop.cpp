#include "strategy_mm_state_stop.h"

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

TaskVoid StrategyMarketMakerStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
    }

    spdlog::info("StrategyMarketMakerStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);

    co_return;
}

// Json StrategyMarketMakerStateStop::get_open_orders()
// {
//     return Json::create_array();
// }