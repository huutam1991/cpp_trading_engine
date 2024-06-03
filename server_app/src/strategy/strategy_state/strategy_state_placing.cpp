#include <strategy/strategy_state/strategy_state_placing.h>
#include <app_utils.h>

StrategyStatePlacing::StrategyStatePlacing(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints)
    : StrategyState(gateway, checkpoints)
{}

void StrategyStatePlacing::begin()
{
    ADD_LOG("StrategyStatePlacing - begin");
}

void StrategyStatePlacing::end()
{
    ADD_LOG("StrategyStatePlacing - end");
}

void StrategyStatePlacing::run(double price)
{
    ADD_LOG("StrategyStatePlacing - run");

    DataModel checkpoint = m_checkpoints->get_checkpoint_by_price(price);
    checkpoint["is_current_checkpoint"] = true;

    // // Buy SPOT order
    // Order buy_spot = get_buy_spot_order_by_checkpoint(checkpoint);
    // AppUtils::instance().get_app_pool()->execute_function([gateway = m_gateway, buy_spot, checkpoint]()
    // {
    //     DataModel cp = checkpoint;

    //     Json response = gateway->place(buy_spot);
    //     cp["positions"]["buy_spot"]["quantity"] = response["quantity"];
    //     cp["positions"]["buy_spot"]["volumn_in_usdt"] = response["volumn_in_usdt"];
    // });

    // Sell Perpetual order
    Order sell_perpetual = get_sell_perpetual_order_by_checkpoint(checkpoint);
    AppUtils::instance().get_app_pool()->execute_function([gateway = m_gateway, sell_perpetual, checkpoint]()
    {
        DataModel cp = checkpoint;
        Json response = gateway->place(sell_perpetual);

        ADD_LOG("sell perpetual response: " << response);
    });

    StrategyState::set_state_status("MONITORING");
}

Order StrategyStatePlacing::get_buy_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double price = checkpoint["info"]["price"];
    double size = checkpoint["size"]["buy_volumn"];
    double quantity = size / price;
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        Order::ExchangeType::SPOT,
        symbol,
        Order::Side::BUY,
        "MARKET",
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
        Order::ExchangeType::PERPETUAL,
        symbol,
        Order::Side::SELL,
        "MARKET",
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}
