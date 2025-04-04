#include <strategy/strategy_state/strategy_state_monitoring.h>
#include <mongo_db/mongo_db.h>

#define MAX_NEIGHBOR_CHECKPOINT 3

StrategyStateMonitoring::StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyState(gateway, checkpoints)
{
}

void StrategyStateMonitoring::begin()
{
    ADD_LOG("StrategyStateMonitoring - begin");
}

void StrategyStateMonitoring::end()
{
    ADD_LOG("StrategyStateMonitoring - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_checkpoints->get_symbol());
}

Order StrategyStateMonitoring::get_limit_buy_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double price = checkpoint["info"]["price"];
    double size = checkpoint["size"]["buy_volumn"];
    double quantity = size / price;
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::ExchangeType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

TaskVoid StrategyStateMonitoring::handle_price_update(double price)
{
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();
    double mark_price = checkpoint["info"]["price"];
    double move_price = checkpoint["size"]["move_price"];

    // Get neighbor checkpoints
    for (int i = 1; i <= MAX_NEIGHBOR_CHECKPOINT; i++)
    {
        double higher_price = mark_price + move_price * i;
        double lower_price = mark_price - move_price * i;

        DataModel lower_checkpoint = m_checkpoints->get_checkpoint_by_price(lower_price);
        OrderId order_id = lower_checkpoint["order_id"];
        if (order_id == 0 || OrderManager::instance().is_valid_order(order_id) == false)
        {
            // Place new limit order
            Order order = get_limit_buy_spot_order_by_checkpoint(lower_checkpoint);
            co_await m_gateway->place(order, Order::Status::NEW);

            // Update [order_id]
            order_id = order.order_id;
            lower_checkpoint["order_id"] = (OrderId)order.order_id;
        }

        if (m_neighbor_checkpoints.find(order_id) == m_neighbor_checkpoints.end())
        {
            m_neighbor_checkpoints.insert(std::make_pair(order_id, lower_checkpoint));
        }
    }

    // // Price go down to lower checkpoint
    // if (price <= mark_price - move_price)
    // {
    //     checkpoint["is_current_checkpoint"] = false;

    //     // Continue with the other checkpoint
    //     StrategyState::set_placing_price(mark_price - move_price);
    //     StrategyState::set_state_status("PLACING");

    //     co_return;
    // }
    // // Price go up to higher checkpoint
    // else if (price >= mark_price + move_price)
    // {
    //     checkpoint["is_current_checkpoint"] = false;

    //     // Continue with the other checkpoint
    //     StrategyState::set_placing_price(mark_price + move_price);
    //     StrategyState::set_state_status("PLACING");

    //     co_return;
    // }

    // // Check to take profit
    // Json strategy_config = MongoDB::instance()
    //     .set_db_and_collection(STRATEGY_DB_NAME, "config")
    //     .find_any();

    // if (strategy_config.has_field("take_profit")) {
    //     double take_profit = strategy_config["take_profit"];
    //     DataModel checkpoint = m_checkpoints->get_checkpoint_can_take_profit(price, take_profit);

    //     // Send close order to take profit
    //     if (checkpoint.is_null() == false) {
    //         co_await send_close_spot_order(checkpoint);
    //     }
    // }

    co_return;
}

TaskVoid StrategyStateMonitoring::handle_order_update(Order& order)
{
    // NEW - do nothing
    if (order.status == Order::Status::NEW)
    {}
    // FILLED - update data to order's checkpoint
    else if (order.status == Order::Status::FILLED)
    {
        if (m_neighbor_checkpoints.find(order.order_id) != m_neighbor_checkpoints.end())
        {
            DataModel& checkpoint = m_neighbor_checkpoints[order.order_id];

            // BUY - update filled data
            if (order.side == Order::Side::BUY)
            {
                checkpoint["positions"]["buy_spot"]["quantity"] = order.output_quantity;
                checkpoint["positions"]["buy_spot"]["volumn_in_usdt"] = order.output_quantity * order.filled_price;
            }
            // SELL - update profit
            else
            {
                // Calculate profit
                double place_volumn_in_usdt = checkpoint["positions"]["buy_spot"]["volumn_in_usdt"];
                double close_volumn_in_usdt = order.output_quantity;
                double profit = close_volumn_in_usdt - place_volumn_in_usdt;

                // Save profit to checkpoint
                double buy_spot_profit = checkpoint["accounting"]["buy_spot_profit"];
                double total_profit = checkpoint["accounting"]["total_profit"];
                checkpoint["accounting"] = Json {
                    {"buy_spot_profit", buy_spot_profit + profit},
                    {"total_profit", total_profit + profit}
                };

                // Close buy spot position
                checkpoint["positions"]["buy_spot"] = Json{
                    {"quantity", 0.0},
                    {"volumn_in_usdt", 0.0},
                };
            }
        }
    }
    // CANCELED - update [order_id] = 0 for order's checkpoint
    else if (order.status == Order::Status::CANCELED)
    {
        if (m_neighbor_checkpoints.find(order.order_id) != m_neighbor_checkpoints.end())
        {
            DataModel checkpoint = m_neighbor_checkpoints[order.order_id];
            checkpoint["order_id"] = 0;
        }
    }

    co_return;
}

TaskVoid StrategyStateMonitoring::run(StrategyData data)
{
    ADD_LOG("StrategyStateMonitoring - run");

    double price;
    if (std::holds_alternative<double>(data))
    {
        double price = std::get<double>(data);
        co_await handle_price_update(price);
    }
    else
    {
        Order order = std::get<Order>(data);
        co_await handle_order_update(order);
    }

    co_return;
}
