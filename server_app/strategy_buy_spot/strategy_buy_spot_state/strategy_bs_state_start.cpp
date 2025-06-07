#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_start.h>

StrategyBuySpotStateStart::StrategyBuySpotStateStart(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyStateFirst(gateway, checkpoints)
{
}

void StrategyBuySpotStateStart::begin()
{
    ADD_LOG("StrategyBuySpotStateStart - begin");
}

void StrategyBuySpotStateStart::end()
{
    ADD_LOG("StrategyBuySpotStateStart - end");
}

TaskVoid StrategyBuySpotStateStart::run(StrategyData data)
{
    ADD_LOG("StrategyBuySpotStateStart - run");

    DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();
    if (current_checkpoint.is_null() == true)
    {
        StrategyStateFirst::set_state_status("PLACING");
    }
    else
    {
        StrategyStateFirst::set_state_status("MONITORING");
    }

    co_return;
}
