#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>

StrategyPriceArbitrageStateRun::StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config)
    : StrategyPriceArbitrageState(gateway, config)
{
}

void StrategyPriceArbitrageStateRun::begin()
{
    ADD_LOG("StrategyPriceArbitrageStateRun - begin");
}

void StrategyPriceArbitrageStateRun::end()
{
    ADD_LOG("StrategyPriceArbitrageStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_config.get_symbol());
}

TaskVoid StrategyPriceArbitrageStateRun::run(StrategyData data)
{
    ADD_LOG("StrategyPriceArbitrageStateRun - run");

    co_return;
}
