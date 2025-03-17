#include <strategy/strategy_state/strategy_state_monitoring.h>
#include <mongo_db/mongo_db.h>

StrategyStateMonitoring::StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyState(gateway, checkpoints)
{
    this->begin();
}

void StrategyStateMonitoring::begin()
{
    ADD_LOG("StrategyStateMonitoring - begin");
}

void StrategyStateMonitoring::end()
{
    ADD_LOG("StrategyStateMonitoring - end");
}

TaskVoid StrategyStateMonitoring::run(Json data)
{
    ADD_LOG("StrategyStateMonitoring - run");

    double price = data["price"];

    DataModel checkpoint = m_checkpoints->get_current_checkpoint();
    double mark_price = checkpoint["info"]["price"];
    double move_price = checkpoint["size"]["move_price"];

    // Price go down to lower checkpoint
    if (price <= mark_price - move_price)
    {
        checkpoint["is_current_checkpoint"] = false;

        // Continue with the other checkpoint
        StrategyState::set_placing_price(mark_price - move_price);
        StrategyState::set_state_status("PLACING");

        co_return;
    }
    // Price go up to higher checkpoint
    else if (price >= mark_price + move_price)
    {
        checkpoint["is_current_checkpoint"] = false;

        // Continue with the other checkpoint
        StrategyState::set_placing_price(mark_price + move_price);
        StrategyState::set_state_status("PLACING");

        co_return;
    }

    // Check to take profit
    Json strategy_config = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "config")
        .find_any();

    if (strategy_config.has_field("take_profit")) {
        double take_profit = strategy_config["take_profit"];
        DataModel checkpoint = m_checkpoints->get_checkpoint_can_take_profit(price, take_profit);

        // Send close order to take profit
        if (checkpoint.is_null() == false) {
            co_await send_close_spot_order(checkpoint);
        }
    }

    co_return;
}
