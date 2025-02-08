#include <strategy/strategy_state/strategy_state_close_all_positions.h>

StrategyStateCloseAllPositions::StrategyStateCloseAllPositions(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyState(gateway, checkpoints)
{
    this->begin();
}

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
    DataModel checkpoint = m_checkpoints->get_one_holding_checkpoint();

    if (checkpoint.is_null() == true)
    {
        ADD_LOG("StrategyStateCloseAllPositions - run: Do nothing as there's no holding checkpoint");
    }
    else
    {
        // Send close spot order
        send_close_spot_order(checkpoint);
    }
}