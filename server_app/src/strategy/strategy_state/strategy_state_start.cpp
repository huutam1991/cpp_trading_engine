#include <strategy/strategy_state/strategy_state_start.h>

StrategyStateStart::StrategyStateStart(Gateway* gateway, CheckPoints* checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStateStart::begin()
{
    ADD_LOG("StrategyStateStart - begin");
}

void StrategyStateStart::end()
{
    ADD_LOG("StrategyStateStart - end");
}

void StrategyStateStart::run()
{
    ADD_LOG("StrategyStateStart - run");
}
