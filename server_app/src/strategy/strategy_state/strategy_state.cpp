#include <strategy/strategy_state/strategy_state.h>

StrategyState::StrategyState(Gateway* gateway, CheckPoints* checkpoints)
    : m_gateway(gateway), m_checkpoints(checkpoints)
{}