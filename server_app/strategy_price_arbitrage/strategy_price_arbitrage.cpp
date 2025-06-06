

#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <gateways/gateway_manager.h>

// StrategyState
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_stop.h>

template<>
std::unordered_map<StrategyState, StrategyStateBase*> StrategyPriceArbitrage::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    std::shared_ptr<Gateway> gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    gateway->subscribe_symbol({"BTCUSDT"});

    strategy_states[StrategyState::RUN] = new StrategyPriceArbitrageStateRun(gateway, get_config_reference());
    strategy_states[StrategyState::STOP] = new StrategyPriceArbitrageStateStop();

    return strategy_states;
}

template<>
void StrategyPriceArbitrage::on_config_change(StrategyPriceArbitrageConfig new_config)
{
    // TBD
    spdlog::debug("Update config for StrategyPriceArbitrage");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};
    }
    else
    {
        m_current_state = StrategyStateData{StrategyState::STOP};
    }
}

// Json StrategyPriceArbitrage::get_orders_chain()
// {
//     Json info = Json::create_array();

//     Json orders = MongoDB::instance()
//         .set_db_and_collection("order", "order_list")
//         .find_many();

//     orders.for_each([&info](Json& order)
//     {
//         if (order["status"] == "FILLED")
//         {
//             order.remove_field("_id");
//             info.push_back(order);
//         }
//     });

//     info.sort([](Json& a, Json& b)
//     {
//         return (size_t)a["order_id"] < (size_t)b["order_id"];
//     });

//     return info;
// }

// Json StrategyPriceArbitrage::get_open_orders()
// {
//     std::unordered_map<PAState, StrategyPriceArbitrageState*>* strategy_states = get_strategy_states();
//     PAState state = m_current_state.object.state;

//     // Run get_open_orders() method of new state
//     if ((*strategy_states).find(state) != (*strategy_states).end())
//     {
//         return (*strategy_states)[state]->get_open_orders();
//     }

//     return Json::create_array();
// }