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

TaskVoid StrategyStateCloseAllPositions::run(StrategyData data)
{
    double price = 0.0;
    if (std::holds_alternative<double>(data))
    {
        price = std::get<double>(data);
    }

    DataModel checkpoint = m_checkpoints->get_one_holding_checkpoint();
    if (checkpoint.is_null() == true)
    {
        ADD_LOG("StrategyStateCloseAllPositions - price: " << price << ", run: Do nothing as there's no holding checkpoint");

        if (std::holds_alternative<Order>(data))
        {
            Order order = std::get<Order>(data);
            ADD_LOG("StrategyStateCloseAllPositions - order: " << order.to_json());
        }
    }
    else
    {
        // Send close spot order
        co_await send_close_spot_order(checkpoint);
    }

    co_return;
}