#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state.h>
#include <app_utils/app_utils.h>

StrategyMeanReversionState::StrategyMeanReversionState(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config)
    : m_gateway(gateway), m_config(config)
{
}

StrategyMeanReversionState::~StrategyMeanReversionState()
{
}

DataModel& StrategyMeanReversionState::get_state_status()
{
    static DataModel state_status = JsonNull();

    if (state_status.is_null())
    {
        // Load from DB
        state_status = DataModel::load_single_data_model(STRATEGY_DB_NAME, "mean_reversion_status");

        // Default status is STOP
        if (state_status.get_data().has_field("status") == false)
        {
            state_status["status"] = "STOP";
        }
    }

    return state_status;
}

void StrategyMeanReversionState::set_state_status(const std::string& status)
{
    DataModel& state_status = StrategyMeanReversionState::get_state_status();
    state_status["status"] = status;
}

void StrategyMeanReversionState::begin()
{
    spdlog::info("StrategyMeanReversionState - begin");
}

void StrategyMeanReversionState::end()
{
    spdlog::info("StrategyMeanReversionState - end");
}

TaskVoid StrategyMeanReversionState::run(StrategyMeanReversionData data)
{
    spdlog::info("StrategyMeanReversionState - run");

    co_return;
}
