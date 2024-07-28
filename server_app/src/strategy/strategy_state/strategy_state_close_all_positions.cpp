#include <strategy/strategy_state/strategy_state_close_all_positions.h>

StrategyStateCloseAllPositions::StrategyStateCloseAllPositions(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStateCloseAllPositions::begin()
{
    ADD_LOG("StrategyStateCloseAllPositions - begin");
}

void StrategyStateCloseAllPositions::end()
{
    ADD_LOG("StrategyStateCloseAllPositions - end");
}

void StrategyStateCloseAllPositions::run(double price)
{
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();

    if (checkpoint.is_null() == true)
    {
        ADD_LOG("StrategyStateCloseAllPositions - run: Do nothing");
    }
    else
    {
        // Send close spot order
        send_close_spot_order(checkpoint);

        // // Send close perpetual order
        // send_close_perpetual_order(checkpoint);

        // Mark current checkpoint is false
        checkpoint["is_current_checkpoint"] = false;
    }
}