#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>
#include <app_utils.h>

StrategyPriceArbitrageState::StrategyPriceArbitrageState(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config)
    : m_gateway(gateway), m_config(config)
{
}

StrategyPriceArbitrageState::~StrategyPriceArbitrageState()
{
}

DataModel& StrategyPriceArbitrageState::get_state_status()
{
    static DataModel state_status = JsonNull();

    if (state_status.is_null())
    {
        // Load from DB
        state_status = DataModel::load_single_data_model(STRATEGY_DB_NAME, "price_arbitrage_status");

        // Default status is STOP
        if (state_status.get_data().has_field("status") == false)
        {
            state_status["status"] = "STOP";
        }
    }

    return state_status;
}

void StrategyPriceArbitrageState::set_state_status(const std::string& status)
{
    DataModel& state_status = StrategyPriceArbitrageState::get_state_status();
    state_status["status"] = status;
}

void StrategyPriceArbitrageState::begin()
{
    ADD_LOG("StrategyPriceArbitrageState - begin");
}

void StrategyPriceArbitrageState::end()
{
    ADD_LOG("StrategyPriceArbitrageState - end");
}

TaskVoid StrategyPriceArbitrageState::run(StrategyData data)
{
    ADD_LOG("StrategyPriceArbitrageState - run");

    co_return;
}

