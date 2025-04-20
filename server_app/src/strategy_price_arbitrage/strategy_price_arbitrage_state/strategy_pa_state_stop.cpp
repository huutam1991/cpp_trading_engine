#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_stop.h>

StrategyPriceArbitrageStateStop::StrategyPriceArbitrageStateStop(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config)
    : StrategyPriceArbitrageState(gateway, config)
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

TaskVoid StrategyPriceArbitrageStateStop::run(StrategyData data)
{
    double price;
    if (std::holds_alternative<double>(data))
    {
        price = std::get<double>(data);
    }

    ADD_LOG("StrategyPriceArbitrageStateStop - run: Do nothing, price: " << price);

    co_return;
}