

#include <strategy_mean_reversion/strategy_mean_reversion.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <time/timer.h>
#include <app_constants.h>
#include <app_utils/app_utils.h>

// StrategyState
#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyMeanReversion::init_states()
{
    // Re-init [m_spread_captures] from config
    m_spread_captures.init_from_config(get_config_reference().spread_capture_config);
    m_spread_captures.reset();

    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // Get ExchangeId from account
    ExchangeId exchange_id = m_config->account->get_exchange_id();
    m_gateway = GatewayManager::instance().get_gateway(exchange_id);

    // Subscribe instrument to gateway
    const Instrument* instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config->symbol);
    m_gateway->subscribe_instruments({instrument});

    strategy_states[StrategyState::RUN] = new StrategyMeanReversionStateRun(m_gateway, get_config_reference(), m_spread_captures);
    strategy_states[StrategyState::STOP] = new StrategyMeanReversionStateStop(m_gateway, get_config_reference(), m_spread_captures);

    return strategy_states;
}

void StrategyMeanReversion::on_config_change(StrategyMeanReversionConfig new_config)
{
    spdlog::warn("StrategyMeanReversion::on_config_change - Config changed for strategy [{}], new config: {}", m_strategy_name, new_config.to_json());
    // Re-init config
    init().start_running_on(event_base);

    // Check start-stop
    if (new_config.is_running)
    {
        run();
    }
    else
    {
        stop();
    }
}

Json StrategyMeanReversion::get_info(Json& params)
{
    return m_states[m_current_state->state]->get_info();
}

void StrategyMeanReversion::run()
{
    m_current_state = StrategyStateData{StrategyState::RUN};
}

void StrategyMeanReversion::stop()
{
    m_current_state = StrategyStateData{StrategyState::STOP};
}
