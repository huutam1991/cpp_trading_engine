#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_stop.h>

StrategyBuySpotStateStop::StrategyBuySpotStateStop(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyStateFirst(gateway, checkpoints)
{
}

void StrategyBuySpotStateStop::begin()
{
    ADD_LOG("StrategyBuySpotStateStop - begin");
}

void StrategyBuySpotStateStop::end()
{
    ADD_LOG("StrategyBuySpotStateStop - end");
}

TaskVoid StrategyBuySpotStateStop::run(StrategyData data)
{
    double price;
    if (std::holds_alternative<double>(data))
    {
        price = std::get<double>(data);
    }

    ADD_LOG("StrategyBuySpotStateStop - run: Do nothing, price: " << price);

    co_return;
}