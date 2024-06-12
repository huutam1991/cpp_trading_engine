#include <strategy/strategy_state/strategy_state_monitoring.h>

StrategyStateMonitoring::StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStateMonitoring::begin()
{
    ADD_LOG("StrategyStateMonitoring - begin");
}

void StrategyStateMonitoring::end()
{
    ADD_LOG("StrategyStateMonitoring - end");
}

void StrategyStateMonitoring::run(double price)
{
    ADD_LOG("StrategyStateMonitoring - run");

    DataModel checkpoint = m_checkpoints->get_current_checkpoint();
    double mark_price = checkpoint["info"]["price"];
    double move_price = checkpoint["size"]["move_price"];

    // Price go down to lower checkpoint
    if (price <= mark_price - move_price)
    {
        // // Only close perpetual order
        // send_close_perpetual_order(checkpoint);

        checkpoint["is_current_checkpoint"] = false;

        // Continue with the other checkpoint
        StrategyState::set_placing_price(mark_price - move_price);
        StrategyState::set_state_status("PLACING");
    }
    // Price go up to higher checkpoint
    else if (price >= mark_price + move_price)
    {
        // Close both orders
        send_close_spot_order(checkpoint);
        // send_close_perpetual_order(checkpoint);

        checkpoint["is_current_checkpoint"] = false;

        // Continue with the other checkpoint
        StrategyState::set_placing_price(mark_price + move_price);
        StrategyState::set_state_status("PLACING");
    }

}
