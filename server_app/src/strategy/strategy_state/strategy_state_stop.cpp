#include <strategy/strategy_state/strategy_state_stop.h>

StrategyStateStop::StrategyStateStop(Gateway* gateway, CheckPoints* checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStateStop::begin()
{
    ADD_LOG("StrategyStateStop - begin");
}

void StrategyStateStop::end()
{
    ADD_LOG("StrategyStateStop - end");
}

void StrategyStateStop::run(double price)
{
    ADD_LOG("StrategyStateStop - run");
}
