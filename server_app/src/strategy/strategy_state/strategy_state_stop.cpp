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
    DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();

    if (current_checkpoint.is_null() == true)
    {
        ADD_LOG("StrategyStateStop - run: Do nothing");
    }
    else
    {
        current_checkpoint["is_current_checkpoint"] = false;
        // TBD -> send close orders
    }
}
