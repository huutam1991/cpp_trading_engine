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
    ADD_LOG("StrategyStateStop - run: Do nothing, price: " << price);

    co_return;
}