#include <strategy/strategy_state/strategy_state_monitoring.h>
#include <mongo_db/mongo_db.h>

#define MAX_NEIGHBOR_CHECKPOINT 10
#define MAX_BUY_ORDER 3
#define MAX_SELL_ORDER 3

StrategyStateMonitoring::StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyStateFirst(gateway, checkpoints)
{
}

void StrategyStateMonitoring::begin()
{
    ADD_LOG("StrategyStateMonitoring - begin");

    Json strategy_config = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "config")
        .find_any();

    if (strategy_config.has_field("take_profit"))
    {
        m_take_profit = (double)strategy_config["take_profit"];
    }

    if (strategy_config.has_field("max_price_to_place"))
    {
        m_max_price_to_place = (double)strategy_config["max_price_to_place"];
    }

    ADD_LOG("StrategyStateMonitoring - take_profit = " << m_take_profit);
    ADD_LOG("StrategyStateMonitoring - max_price_to_place = " << m_max_price_to_place);
}

void StrategyStateMonitoring::end()
{
    ADD_LOG("StrategyStateMonitoring - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_checkpoints->get_symbol());
}

void StrategyStateMonitoring::remove_open_order_id(OrderId order_id)
{
    if (m_checkpoint_by_open_order_id.find(order_id) != m_checkpoint_by_open_order_id.end())
    {
        m_checkpoint_by_open_order_id.erase(order_id);
    }
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

Order StrategyStateMonitoring::get_limit_sell_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double price = checkpoint["info"]["price"];
    double quantity = checkpoint["positions"]["buy_spot"]["quantity"];
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::ExchangeType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::SELL,
        Order::OrderType::LIMIT,
        price + m_take_profit,
        round_up_quantity
    );
}

TaskVoid StrategyStateMonitoring::check_place_buy_order(double price)
{
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();
    double mark_price = checkpoint["info"]["price"];
    double move_price = checkpoint["size"]["move_price"];

    // Get loop through neighbor checkpoints
    for (int i = 0; i <= MAX_NEIGHBOR_CHECKPOINT; i++)
    {
        double lower_price = mark_price - move_price * i;

        DataModel checkpoint = m_checkpoints->get_checkpoint_by_price(lower_price);
        OrderId order_id = checkpoint["buy_order_id"];

        // Check place buy order
        if (i <= MAX_BUY_ORDER)
        {
            if (order_id == 0 || OrderManager::instance().is_valid_order(order_id) == false)
            {
                // Place new limit order
                Order order = get_limit_buy_spot_order_by_checkpoint(checkpoint);
                // co_await m_gateway->place(order, Order::Status::NEW);

                // Update [buy_order_id]
                order_id = order.order_id;
                checkpoint["buy_order_id"] = (OrderId)order.order_id;
            }

            if (m_checkpoint_by_open_order_id.find(order_id) == m_checkpoint_by_open_order_id.end())
            {
                m_checkpoint_by_open_order_id.insert(std::make_pair(order_id, checkpoint));
            }
        }
        // Check to close buy order (because these orders are at too low price, hard to get filled)
        else
        {
            if (order_id != 0 && OrderManager::instance().is_valid_order(order_id) == true)
            {
                // Check if this is a NEW order (hasn't got filled yet)
                Order& order = OrderManager::instance().get_order_by_id(order_id);
                if (order.status == Order::Status::NEW)
                {
                    // Close this order
                    m_gateway->cancel(order);

                    // Remove [order_id] from [m_checkpoint_by_open_order_id]
                    remove_open_order_id(order_id);
                }
            }
        }

    }

    co_return;
}

TaskVoid StrategyStateMonitoring::check_place_sell_order(double price)
{
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();
    double mark_price = checkpoint["info"]["price"];
    double move_price = checkpoint["size"]["move_price"];

    int sell_orders_count = 0;

    // Get loop through neighbor checkpoints
    for (int i = MAX_NEIGHBOR_CHECKPOINT; i >= -MAX_SELL_ORDER; i--)
    {
        double price = mark_price - move_price * i;

        DataModel checkpoint = m_checkpoints->get_checkpoint_by_price(price);
        OrderId buy_order_id = checkpoint["buy_order_id"];

        // Check if this checkpoint has buy order is filled
        if (buy_order_id != 0 &&
            (double)checkpoint["positions"]["buy_spot"]["quantity"] > 0.0 &&
            (double)checkpoint["positions"]["buy_spot"]["volumn_in_usdt"] > 0.0)
        {
            OrderId sell_order_id = checkpoint["sell_order_id"];
            sell_orders_count++;

            if (sell_orders_count <= MAX_SELL_ORDER)
            {
                if (sell_order_id == 0 || OrderManager::instance().is_valid_order(sell_order_id) == false)
                {
                    // Place new limit order
                    Order order = get_limit_sell_spot_order_by_checkpoint(checkpoint);
                    // co_await m_gateway->place(order, Order::Status::NEW);

                    // Update [buy_order_id]
                    sell_order_id = order.order_id;
                    checkpoint["sell_order_id"] = (OrderId)order.order_id;
                }

                if (m_checkpoint_by_open_order_id.find(sell_order_id) == m_checkpoint_by_open_order_id.end())
                {
                    m_checkpoint_by_open_order_id.insert(std::make_pair(sell_order_id, checkpoint));
                }
            }
            // Check to cancel Sell order (because these orders are at too high price, hard to get filled)
            else
            {
                if (sell_order_id != 0 && OrderManager::instance().is_valid_order(sell_order_id) == true)
                {
                    // Check if this is a NEW order (hasn't got filled yet)
                    Order& order = OrderManager::instance().get_order_by_id(sell_order_id);
                    if (order.status == Order::Status::NEW)
                    {
                        // Close this order
                        m_gateway->cancel(order);

                        // Remove [sell_order_id] from [m_checkpoint_by_open_order_id]
                        remove_open_order_id(sell_order_id);
                    }
                }
            }
        }
    }

    co_return;
}

TaskVoid StrategyStateMonitoring::handle_price_update(double price)
{
    // Update current checkpoint
    DataModel checkpoint = m_checkpoints->get_current_checkpoint();
    double mark_price = checkpoint["info"]["price"];
    double move_price = checkpoint["size"]["move_price"];

    // Price go down to lower checkpoint
    if (price <= mark_price - move_price)
    {
        checkpoint["is_current_checkpoint"] = false;
        DataModel new_checkpoint = m_checkpoints->get_checkpoint_by_price(mark_price - move_price);
        new_checkpoint["is_current_checkpoint"] = true;

        co_return;
    }
    // Price go up to higher checkpoint
    else if (price >= mark_price + move_price)
    {
        checkpoint["is_current_checkpoint"] = false;
        DataModel new_checkpoint = m_checkpoints->get_checkpoint_by_price(mark_price + move_price);
        new_checkpoint["is_current_checkpoint"] = true;

        co_return;
    }

    // Check [m_max_price_to_place]
    if (price >= m_max_price_to_place)
    {
        co_return;
    }

    co_await check_place_buy_order(price);
    co_await check_place_sell_order(price);

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
        if (m_checkpoint_by_open_order_id.find(order.order_id) != m_checkpoint_by_open_order_id.end())
        {
            DataModel& checkpoint = m_checkpoint_by_open_order_id[order.order_id];

            // BUY - open order - update filled data
            if (order.side == Order::Side::BUY)
            {
                checkpoint["positions"]["buy_spot"] = Json {
                    {"quantity", order.output_quantity},
                    {"volumn_in_usdt", order.volumn_in_quote_currency}
                };
            }
            // SELL - close order - update profit
            else
            {
                // Calculate profit
                double place_volumn_in_usdt = checkpoint["positions"]["buy_spot"]["volumn_in_usdt"];
                double close_volumn_in_usdt = order.volumn_in_quote_currency;
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

                // Update [buy_order_id] and [sell_order_id] to 0
                checkpoint["buy_order_id"] = 0;
                checkpoint["sell_order_id"] = 0;
            }

            remove_open_order_id(order.order_id);
        }
    }
    // CANCELED - update [order_id] = 0 for order's checkpoint
    else if (order.status == Order::Status::CANCELED)
    {
        if (m_checkpoint_by_open_order_id.find(order.order_id) != m_checkpoint_by_open_order_id.end())
        {
            DataModel checkpoint = m_checkpoint_by_open_order_id[order.order_id];

            if (order.side == Order::Side::BUY)
            {
                checkpoint["buy_order_id"] = 0;
            }
            else
            {
                checkpoint["sell_order_id"] = 0;
            }

            remove_open_order_id(order.order_id);
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
