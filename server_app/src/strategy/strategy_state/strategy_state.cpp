#include <strategy/strategy_state/strategy_state.h>

StrategyState::StrategyState(Gateway* gateway, CheckPoints* checkpoints)
    : m_gateway(gateway), m_checkpoints(checkpoints)
{
    this->begin();
}

StrategyState::~StrategyState()
{
    this->end();
}

DataModel StrategyState::get_state_status()
{
    static DataModel state_status = JsonNull();

    if (state_status.is_null())
    {
        // Load from DB
        state_status = DataModel::get_single_data_model(STRATEGY_DB_NAME, "status");

        // Default status is STOP
        if (state_status.get_data().has_field("status") == false)
        {
            state_status["status"] = "STOP";
        }
    }

    return state_status;
}

void StrategyState::set_state_status(const std::string& status)
{
    DataModel state_status = StrategyState::get_state_status();
    state_status["status"] = status;
}

void StrategyState::begin()
{
    ADD_LOG("StrategyState - begin");
}

void StrategyState::end()
{
    ADD_LOG("StrategyState - end");
}

void StrategyState::run(double price)
{
    ADD_LOG("StrategyState - run");
}
