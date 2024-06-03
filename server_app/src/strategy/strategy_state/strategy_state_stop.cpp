#include <strategy/strategy_state/strategy_state_stop.h>
#include <app_utils.h>

StrategyStateStop::StrategyStateStop(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints)
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
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();

    if (checkpoint.is_null() == true)
    {
        ADD_LOG("StrategyStateStop - run: Do nothing");
    }
    else
    {
        // Send close spot order
        Order close_buy_spot = get_close_buy_spot_order_by_checkpoint(checkpoint);
        AppUtils::instance().get_app_pool()->execute_function([gateway = m_gateway, close_buy_spot, checkpoint]()
        {
            DataModel cp = checkpoint;

            Json response = gateway->place(close_buy_spot);

            ADD_LOG("Close buy SPOT order: " << response);
        });

        // Mark current checkpoint is false
        checkpoint["is_current_checkpoint"] = false;
    }
}

Order StrategyStateStop::get_close_buy_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double quantity = checkpoint["positions"]["buy_spot"];
    double round_up_quantity = m_gateway->round_up_quantity(symbol, quantity);

    return Order(
        Order::ExchangeType::SPOT,
        symbol,
        Order::Side::SELL,
        "MARKET",
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyStateStop::get_close_sell_perpetual_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double quantity = checkpoint["positions"]["sell_perpetual"];
    double round_up_quantity = m_gateway->round_up_quantity(symbol, quantity);

    return Order(
        Order::ExchangeType::PERPETUAL,
        symbol,
        Order::Side::BUY,
        "MARKET",
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}
