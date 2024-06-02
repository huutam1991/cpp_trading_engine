#include <strategy/strategy_state/strategy_state_placing.h>

StrategyStatePlacing::StrategyStatePlacing(Gateway* gateway, CheckPoints* checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStatePlacing::begin()
{
    ADD_LOG("StrategyStatePlacing - begin");
}

void StrategyStatePlacing::end()
{
    ADD_LOG("StrategyStatePlacing - end");
}

void StrategyStatePlacing::run(double price)
{
    ADD_LOG("StrategyStatePlacing - run");

    DataModel checkpoint = m_checkpoints->get_checkpoint_by_price(price);
    checkpoint["is_current_checkpoint"] = true;
}
