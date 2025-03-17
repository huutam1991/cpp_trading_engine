#include <strategy/strategy_state/strategy_state_start.h>

StrategyStateStart::StrategyStateStart(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyState(gateway, checkpoints)
{
    this->begin();
}

void StrategyStateStart::begin()
{
    ADD_LOG("StrategyStateStart - begin");
}

void StrategyStateStart::end()
{
    ADD_LOG("StrategyStateStart - end");
}

TaskVoid StrategyStateStart::run(Json data)
{
    ADD_LOG("StrategyStateStart - run");

    DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();
    if (current_checkpoint.is_null() == true)
    {
        StrategyState::set_state_status("PLACING");
    }
    else
    {
        StrategyState::set_state_status("MONITORING");
    }

    co_return;
}
