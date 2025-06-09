

#include <strategy_price_arbitrage/strategy_price_arbitrage.h>

// StrategyState
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_stop.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyPriceArbitrage::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    m_gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    m_gateway->subscribe_symbol({m_config.object.symbol_1, m_config.object.symbol_2});

    strategy_states[StrategyState::RUN] = new StrategyPriceArbitrageStateRun(m_gateway, get_config_reference());
    strategy_states[StrategyState::STOP] = new StrategyPriceArbitrageStateStop();

    return strategy_states;
}

void StrategyPriceArbitrage::on_config_change(StrategyPriceArbitrageConfig new_config)
{
    spdlog::debug("Update config for StrategyPriceArbitrage");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};
    }
    else
    {
        m_current_state = StrategyStateData{StrategyState::STOP};
    }
    
    // Re-subscribe symbols
    m_gateway->subscribe_symbol({m_config.object.symbol_1, m_config.object.symbol_2});
}

Json StrategyPriceArbitrage::get_info(Json& params)
{
    if ((std::string)params["type"] == "orders_chain")
    {
        return get_orders_chain();
    }

    return {};
}

Json StrategyPriceArbitrage::get_orders_chain()
{
    Json orders = Json::create_array();

    Json filled_orders = MongoDB::instance()
        .set_db_and_collection("order", "order_list")
        .find_many();

    filled_orders.for_each([&orders](Json& order)
    {
        if (order["status"] == "FILLED")
        {
            order.remove_field("_id");
            orders.push_back(order);
        }
    });

    orders.sort([](Json& a, Json& b)
    {
        return (OrderId)a["order_id"] < (OrderId)b["order_id"];
    });

    Json res = Json::create_array();
    size_t i = 0;
    while (i < orders.size())
    {   
        // Find triangle orders
        if ((std::string)orders[i]["symbol"] == m_config.object.symbol_1 && 
            (std::string)orders[i + 1]["symbol"] == m_config.object.symbol_2 && 
            (std::string)orders[i + 2]["symbol"] == m_config.object.symbol_3)
        {
            double input = (double)orders[i]["volumn_in_quote_currency"];
            double output = (double)orders[i+2]["output_quantity"];
            
            Json triangle;
            triangle["input"] = input;
            triangle["output"] = output;
            triangle["profit"] = output - input;
            triangle["orders"] = {
                {m_config.object.symbol_1, orders[i]["order_id"]},
                {m_config.object.symbol_2, orders[i + 1]["order_id"]},
                {m_config.object.symbol_3, orders[i + 2]["order_id"]}
            };

            res.push_back(triangle);
        }

        i++;
    }

    return res;
}

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