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
        std::vector<DataModel> load = DataModel::get_data_model_list(STRATEGY_DB_NAME, "status");
        if (load.size() > 0)
        {
            state_status = load[0];
        }
        else
        {
            state_status = DataModel(STRATEGY_DB_NAME, "status");
            state_status["status"] = "STOP";
        }
    }

    return state_status;
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
