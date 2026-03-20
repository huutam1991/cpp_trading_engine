

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

    // For now, only use Binance
    m_gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);
    const Instrument* instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.object.symbol);
    m_gateway->subscribe_instruments({instrument});

    strategy_states[StrategyState::RUN] = new StrategyMeanReversionStateRun(m_gateway, get_config_reference(), m_spread_captures);
    strategy_states[StrategyState::STOP] = new StrategyMeanReversionStateStop(m_gateway, get_config_reference(), m_spread_captures);

    return strategy_states;
}

void StrategyMeanReversion::on_config_change(StrategyMeanReversionConfig new_config)
{
    // Re-init config
    init();

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
    return m_states[m_current_state.object.state]->get_info();
}

void StrategyMeanReversion::run()
{
    m_current_state = StrategyStateData{StrategyState::RUN};
}

void StrategyMeanReversion::stop()
{
    m_current_state = StrategyStateData{StrategyState::STOP};
}
