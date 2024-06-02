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

void StrategyStateStart::run(double price)
{
    ADD_LOG("StrategyStateStart - run");

    DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();
    if (current_checkpoint.is_null() == true)
    {
        StrategyState::set_state_status("PLACING");
    }
}
