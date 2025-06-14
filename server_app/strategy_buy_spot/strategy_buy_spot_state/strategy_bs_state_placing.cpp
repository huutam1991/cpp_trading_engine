#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_placing.h>
#include <app_utils/app_utils.h>

StrategyStatePlacing::StrategyStatePlacing(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : StrategyStateFirst(gateway, checkpoints)
{
}

void StrategyStatePlacing::begin()
{
    ADD_LOG("StrategyStatePlacing - begin");
}

void StrategyStatePlacing::end()
{
    ADD_LOG("StrategyStatePlacing - end");
}

TaskVoid StrategyStatePlacing::run(StrategyData data)
{
    ADD_LOG("StrategyStatePlacing - run");

    // Get [price] to place
    double placing_price = StrategyStateFirst::get_placing_price();
    double price;
    if (std::holds_alternative<double>(data))
    {
        price = std::get<double>(data);
    }
    else
    {
        // Currently, only handle price update
        co_return;
    }
    price = placing_price == -1 ? price : placing_price;

    DataModel checkpoint = m_checkpoints->get_checkpoint_by_price(price);
    checkpoint["is_current_checkpoint"] = true;

    // Check [max_price_to_place]
    Json strategy_config = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "config")
        .find_any();

    if (strategy_config.has_field("max_price_to_place"))
    {
        double max_price_to_place = strategy_config["max_price_to_place"];

        if (price >= max_price_to_place)
        {
            StrategyStateFirst::set_state_status("MONITORING");

            co_return;
        }
    }

    // Increase visit times
    long visit_times = checkpoint["accounting"]["visit_times"];
    checkpoint["accounting"]["visit_times"] = visit_times + 1;

    // Buy SPOT order
    Order buy_spot = get_buy_spot_order_by_checkpoint(checkpoint);

    // Only place buy spot if this checkpoint is not holding any quantity
    double quantity = checkpoint["positions"]["buy_spot"]["quantity"];
    if (quantity == 0)
    {
        // Order order = co_await m_gateway->place(buy_spot);

        // checkpoint["positions"]["buy_spot"]["quantity"] = order.output_quantity;
        // checkpoint["positions"]["buy_spot"]["volumn_in_usdt"] = order.output_quantity * order.filled_price;

        // checkpoint["buy_order_id"] = (OrderId)order.order_id;
    }

    // // Sell Perpetual order
    // Order sell_perpetual = get_sell_perpetual_order_by_checkpoint(checkpoint);
    // AppUtils::instance().get_app_pool()->execute_function([gateway = m_gateway, sell_perpetual, checkpoint]()
    // {
    //     DataModel cp = checkpoint;

    //     Json response = gateway->place(sell_perpetual);
    //     cp["positions"]["sell_perpetual"]["quantity"] = response["quantity"];
    //     cp["positions"]["sell_perpetual"]["volumn_in_usdt"] = response["volumn_in_usdt"];

    //     ADD_LOG("sell perpetual response: " << response);
    // });

    StrategyStateFirst::set_state_status("MONITORING");

    co_return;
}

Order StrategyStatePlacing::get_buy_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double price = checkpoint["info"]["price"];
    double size = checkpoint["size"]["buy_volumn"];
    double quantity = size / price;
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NEW,
        symbol,
        symbol,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyStatePlacing::get_sell_perpetual_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double price = checkpoint["info"]["price"];
    double buy_size = checkpoint["size"]["buy_volumn"];
    double sell_buy_ratio = checkpoint["size"]["sell_buy_ratio"];
    double sell_size = sell_buy_ratio * buy_size;
    double quantity = sell_size / price;
    double round_up_quantity = m_gateway->round_up_quantity("perpetual", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::PERPETUAL,
        Order::Status::NEW,
        symbol,
        symbol,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        price,
        round_up_quantity
    );
}
