#include <strategy_buy_spot/strategy_buy_spot_state/strategy_state.h>
#include <app_utils/app_utils.h>

StrategyStateFirst::StrategyStateFirst(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints)
    : m_gateway(gateway), m_checkpoints(checkpoints)
{
}

StrategyStateFirst::~StrategyStateFirst()
{
}

DataModel& StrategyStateFirst::get_state_status()
{
    static DataModel state_status = JsonNull();

    if (state_status.is_null())
    {
        // Load from DB
        state_status = DataModel::load_single_data_model(STRATEGY_DB_NAME, "status");

        // Default status is STOP
        if (state_status.get_data().has_field("status") == false)
        {
            state_status["status"] = "STOP";
        }
    }

    return state_status;
}

void StrategyStateFirst::set_state_status(const std::string& status)
{
    DataModel& state_status = StrategyStateFirst::get_state_status();
    state_status["status"] = status;
}

void StrategyStateFirst::begin()
{
    ADD_LOG("StrategyStateFirst - begin");
}

void StrategyStateFirst::end()
{
    ADD_LOG("StrategyStateFirst - end");
}

TaskVoid StrategyStateFirst::run(StrategyData data)
{
    ADD_LOG("StrategyStateFirst - run");

    co_return;
}

double* StrategyStateFirst::placing_price_ptr()
{
    static double placing_price = -1;
    return &placing_price;
}

void StrategyStateFirst::set_placing_price(double price)
{
    double* price_ptr = placing_price_ptr();
    *price_ptr = price;
}

double StrategyStateFirst::get_placing_price()
{
    double* price_ptr = placing_price_ptr();
    return *price_ptr;
}

TaskVoid StrategyStateFirst::send_close_spot_order(DataModel& checkpoint)
{
    Order close_buy_spot = get_close_buy_spot_order_by_checkpoint(checkpoint);

    // Place close buy spot order
    // Order response = co_await m_gateway->place(close_buy_spot);

    // Calculate profit
    double place_volumn_in_usdt = checkpoint["positions"]["buy_spot"]["volumn_in_usdt"];
    // double close_volumn_in_usdt = response.output_quantity;
    double close_volumn_in_usdt = 0; // Need fix
    double profit = close_volumn_in_usdt - place_volumn_in_usdt;

    // Save profit to checkpoint
    double buy_spot_profit = checkpoint["accounting"]["buy_spot_profit"];
    double total_profit = checkpoint["accounting"]["total_profit"];
    checkpoint["accounting"]["buy_spot_profit"] = buy_spot_profit + profit;
    checkpoint["accounting"]["total_profit"] = total_profit + profit;

    // Close buy spot position
    checkpoint["positions"]["buy_spot"] = Json{
        {"quantity", 0.0},
        {"volumn_in_usdt", 0.0},
    };

    checkpoint["is_current_checkpoint"] = false;

    co_return;
}

void StrategyStateFirst::send_close_perpetual_order(DataModel& checkpoint)
{
    Order close_sell_perpetual = get_close_sell_perpetual_order_by_checkpoint(checkpoint);
    AppUtils::instance().get_app_pool()->execute_function([gateway = m_gateway, close_sell_perpetual, checkpoint]()
    {
        DataModel cp = checkpoint;

        // Place close buy spot order
        // Json response = gateway->place(close_sell_perpetual);
        Json response;

        // Calculate profit
        double place_volumn_in_usdt = cp["positions"]["sell_perpetual"]["volumn_in_usdt"];
        double close_volumn_in_usdt = response["volumn_in_usdt"];
        double profit = place_volumn_in_usdt - close_volumn_in_usdt;

        // Save profit to checkpoint
        double sell_perpetual_profit = cp["accounting"]["sell_perpetual_profit"];
        double total_profit = cp["accounting"]["total_profit"];
        cp["accounting"]["sell_perpetual_profit"] = sell_perpetual_profit + profit;
        cp["accounting"]["total_profit"] = total_profit + profit;

        // Close buy spot position
        cp["positions"]["sell_perpetual"] = Json{
            {"quantity", 0.0},
            {"volumn_in_usdt", 0.0},
        };
    });
}

Order StrategyStateFirst::get_close_buy_spot_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double quantity = checkpoint["positions"]["buy_spot"]["quantity"];
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NEW,
        symbol,
        symbol,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyStateFirst::get_close_sell_perpetual_order_by_checkpoint(DataModel& checkpoint)
{
    std::string symbol = checkpoint["info"]["symbol"];
    double quantity = checkpoint["positions"]["sell_perpetual"]["quantity"];
    double round_up_quantity = m_gateway->round_up_quantity("perpetual", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::PERPETUAL,
        Order::Status::NEW,
        symbol,
        symbol,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}
