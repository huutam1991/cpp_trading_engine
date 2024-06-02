#include <strategy/strategy_state/strategy_state.h>

StrategyState::StrategyState(Gateway* gateway, CheckPoints* checkpoints)
    : m_gateway(gateway), m_checkpoints(checkpoints)
{
    this->begin();
}

StrategyState::~StrategyState()
{
    this->end();
}

void StrategyState::begin()
{
    ADD_LOG("StrategyState - begin");
}

void StrategyState::end()
{
    ADD_LOG("StrategyState - end");
}

void StrategyState::run(double price)
{
    ADD_LOG("StrategyState - run");
}
