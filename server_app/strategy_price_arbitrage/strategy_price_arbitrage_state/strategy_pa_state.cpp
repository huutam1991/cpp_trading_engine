#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>
#include <app_utils/app_utils.h>

StrategyPriceArbitrageState::StrategyPriceArbitrageState(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config)
    : m_gateway(gateway), m_config(config)
{
}

StrategyPriceArbitrageState::~StrategyPriceArbitrageState()
{
}

void StrategyPriceArbitrageState::begin()
{
    ADD_LOG("StrategyPriceArbitrageState - begin");
}

void StrategyPriceArbitrageState::end()
{
    ADD_LOG("StrategyPriceArbitrageState - end");
}

TaskVoid StrategyPriceArbitrageState::run(StrategyPriceArbitrageData data)
{
    ADD_LOG("StrategyPriceArbitrageState - run");

    co_return;
}
