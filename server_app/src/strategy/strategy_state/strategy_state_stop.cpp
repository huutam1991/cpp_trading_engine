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

            // Place close buy spot order
            Json response = gateway->place(close_buy_spot);

            // Calculate profit
            double place_volumn_in_usdt = cp["positions"]["buy_spot"]["volumn_in_usdt"];
            double close_volumn_in_usdt = response["volumn_in_usdt"];
            double profit = close_volumn_in_usdt - place_volumn_in_usdt;

            // Save profit to checkpoint
            double buy_spot_profit = cp["buy_spot_profit"];
            double total_profit = cp["total_profit"];
            cp["accounting"]["buy_spot_profit"] = buy_spot_profit + profit;
            cp["accounting"]["total_profit"] = total_profit + profit;

            // Close buy spot position
            cp["positions"]["buy_spot"] = Json{
                {"quantity", 0.0},
                {"volumn_in_usdt", 0.0},
            };
        });

        // Send close perpetual order
        Order close_sell_perpetual = get_close_sell_perpetual_order_by_checkpoint(checkpoint);
        AppUtils::instance().get_app_pool()->execute_function([gateway = m_gateway, close_sell_perpetual, checkpoint]()
        {
            DataModel cp = checkpoint;

            // Place close buy spot order
            Json response = gateway->place(close_sell_perpetual);

            // Calculate profit
            double place_volumn_in_usdt = cp["positions"]["sell_perpetual"]["volumn_in_usdt"];
            double close_volumn_in_usdt = response["volumn_in_usdt"];
            double profit = close_volumn_in_usdt - place_volumn_in_usdt;

            // Save profit to checkpoint
            double sell_perpetual_profit = cp["sell_perpetual_profit"];
            double total_profit = cp["total_profit"];
            cp["accounting"]["sell_perpetual_profit"] = sell_perpetual_profit + profit;
            cp["accounting"]["total_profit"] = total_profit + profit;

            // Close buy spot position
            cp["positions"]["sell_perpetual"] = Json{
                {"quantity", 0.0},
                {"volumn_in_usdt", 0.0},
            };
        });

        // Mark current checkpoint is false
        checkpoint["is_current_checkpoint"] = false;
    }
}

Order StrategyStateStop::get_close_buy_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double quantity = checkpoint["positions"]["buy_spot"]["quantity"];
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

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
    double quantity = checkpoint["positions"]["sell_perpetual"]["quantity"];
    double round_up_quantity = m_gateway->round_up_quantity("perpetual", symbol, quantity);

    return Order(
        Order::ExchangeType::PERPETUAL,
        symbol,
        Order::Side::BUY,
        "MARKET",
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}
