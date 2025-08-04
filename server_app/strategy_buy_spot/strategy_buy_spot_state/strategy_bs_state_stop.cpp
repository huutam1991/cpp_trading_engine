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
    }

    spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);

    co_return;
}

// Json StrategyBuySpotStateStop::get_open_orders()
// {
//     return Json::create_array();
// }