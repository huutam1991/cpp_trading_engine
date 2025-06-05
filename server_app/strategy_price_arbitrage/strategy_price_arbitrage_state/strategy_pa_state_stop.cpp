#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_stop.h>

StrategyPriceArbitrageStateStop::StrategyPriceArbitrageStateStop()
{
}

void StrategyPriceArbitrageStateStop::begin()
{
    ADD_LOG("StrategyPriceArbitrageStateStop - begin");
}

void StrategyPriceArbitrageStateStop::end()
{
    ADD_LOG("StrategyPriceArbitrageStateStop - end");
}

TaskVoid StrategyPriceArbitrageStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
    }

    spdlog::info("StrategyPriceArbitrageStateStop - run: Do nothing, symbol: {}, price: {} ", price_update.symbol, price_update.price);

    co_return;
}

// Json StrategyPriceArbitrageStateStop::get_open_orders()
// {
//     return Json::create_array();
// }