#include <strategy/strategy_state/strategy_state_monitoring.h>

StrategyStateMonitoring::StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStateMonitoring::begin()
{
    ADD_LOG("StrategyStateMonitoring - begin");
}

void StrategyStateMonitoring::end()
{
    ADD_LOG("StrategyStateMonitoring - end");
}

void StrategyStateMonitoring::run(double price)
{
    ADD_LOG("StrategyStateMonitoring - run: Do nothing");
}
