#include <strategy/strategy_state/strategy_state_stop.h>

StrategyStateStop::StrategyStateStop(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyState(gateway, checkpoints)
{
    this->begin();
}

void StrategyStateStop::begin()
{
    ADD_LOG("StrategyStateStop - begin");
}

void StrategyStateStop::end()
{
    ADD_LOG("StrategyStateStop - end");
}

TaskVoid StrategyStateStop::run(double price)
{
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();

    if (checkpoint.is_null() == true)
    {
        ADD_LOG("StrategyStateStop - run: Do nothing, price: " << price);
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

    co_return;
}