

#include <strategy_buy_spot/strategy_buy_spot.h>

// StrategyState
#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_stop.h>
#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_run.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyBuySpot::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    m_gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);

    strategy_states[StrategyState::STOP] = new StrategyBuySpotStateStop();
    strategy_states[StrategyState::RUN] = new StrategyBuySpotStateRun(m_gateway, get_config_reference());

    return strategy_states;
}

void StrategyBuySpot::start()
{
    // Subscribe symbols
    auto instrument = m_gateway->get_instrument_by_symbol(m_config.object.symbol);
    m_gateway->subscribe_symbol({instrument->exchange_symbol});
}

void StrategyBuySpot::on_config_change(StrategyBuySpotConfig new_config)
{
    spdlog::debug("Update config for StrategyBuySpot");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};
    }
    else
    {
        m_current_state = StrategyStateData{StrategyState::STOP};
    }
}

Json StrategyBuySpot::get_info(Json& params)
{
    if ((std::string)params["type"] == "orders_chain")
    {
        // return get_orders_chain();
    }

    return {};
}

// Json StrategyBuySpot::get_orders_chain()
// {
//     std::string symbol_1 = m_gateway->get_instrument_by_symbol(m_config.object.symbol_1)->exchange_symbol;
//     std::string symbol_2 = m_gateway->get_instrument_by_symbol(m_config.object.symbol_2)->exchange_symbol;
//     std::string symbol_3 = m_gateway->get_instrument_by_symbol(m_config.object.symbol_3)->exchange_symbol;

//     Json orders = Json::create_array();

//     Json filled_orders = MongoDB::instance()
//         .set_db_and_collection("order", "order_list")
//         .find_many();

//     filled_orders.for_each([&orders](Json& order)
//     {
//         if (order["status"] == "FILLED")
//         {
//             order.remove_field("_id");
//             orders.push_back(order);
//         }
//     });

//     orders.sort([](Json& a, Json& b)
//     {
//         return (OrderId)a["order_id"] < (OrderId)b["order_id"];
//     });

//     Json res = Json::create_array();
//     size_t i = 0;
//     while (i < orders.size())
//     {   
//         // Find triangle orders
//         if ((std::string)orders[i]["symbol"] == symbol_1 && 
//             (std::string)orders[i + 1]["symbol"] == symbol_2 && 
//             (std::string)orders[i + 2]["symbol"] == symbol_3)
//         {
//             double input = (double)orders[i]["volumn_in_quote_currency"];
//             double output = (double)orders[i+2]["output_quantity"];
            
//             Json triangle;
//             triangle["input"] = input;
//             triangle["output"] = output;
//             triangle["profit"] = output - input;
//             triangle["orders"] = {
//                 {symbol_1, orders[i]["order_id"]},
//                 {symbol_2, orders[i + 1]["order_id"]},
//                 {symbol_3, orders[i + 2]["order_id"]}
//             };

//             res.push_back(triangle);
//         }

//         i++;
//     }

//     return res;
// }

// Json StrategyBuySpot::get_open_orders()
// {
//     std::unordered_map<PAState, StrategyBuySpotState*>* strategy_states = get_strategy_states();
//     PAState state = m_current_state.object.state;

//     // Run get_open_orders() method of new state
//     if ((*strategy_states).find(state) != (*strategy_states).end())
//     {
//         return (*strategy_states)[state]->get_open_orders();
//     }

//     return Json::create_array();
// }