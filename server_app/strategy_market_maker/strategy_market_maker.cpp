

#include <strategy_market_maker/strategy_market_maker.h>
#include <gateways/gateway_manager.h>

// StrategyState
#include <strategy_market_maker/strategy_market_maker_state/strategy_mm_state_run.h>
#include <strategy_market_maker/strategy_market_maker_state/strategy_mm_state_stop.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyMarketMaker::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    std::shared_ptr<Gateway> gateway = GatewayManager::instance().get_gateway(ExchangeEnum::BINANCE);
    gateway->subscribe_symbol({m_config.object.symbol});

    strategy_states[StrategyState::RUN] = new StrategyMarketMakerStateRun(gateway, get_config_reference());
    strategy_states[StrategyState::STOP] = new StrategyMarketMakerStateStop();

    return strategy_states;
}

void StrategyMarketMaker::on_config_change(StrategyMarketMakerConfig new_config)
{
    spdlog::debug("Update config for StrategyMarketMaker");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};

        // Update config for state Run
        auto state_run = (StrategyMarketMakerStateRun*)m_states[StrategyState::RUN];
        state_run->on_config_change();
    }
    else
    {
        m_current_state = StrategyStateData{StrategyState::STOP};
    }
}

Json StrategyMarketMaker::get_info(Json& params)
{
    if ((std::string)params["type"] == "orders_chain")
    {
        return get_orders_chain();
    }

    return {};
}

Json StrategyMarketMaker::get_orders_chain()
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
        if ((std::string)orders[i]["symbol"] == "BTCUSDT" && 
            (std::string)orders[i + 1]["symbol"] == "ETHBTC" && 
            (std::string)orders[i + 2]["symbol"] == "ETHUSDT")
        {
            double input = (double)orders[i]["volumn_in_quote_currency"];
            double output = (double)orders[i+2]["output_quantity"];
            
            Json triangle;
            triangle["input"] = input;
            triangle["output"] = output;
            triangle["profit"] = output - input;
            triangle["orders"] = {
                {"BTCUSDT", orders[i]["order_id"]},
                {"ETHBTC", orders[i + 1]["order_id"]},
                {"ETHUSDT", orders[i + 2]["order_id"]}
            };

            res.push_back(triangle);
        }

        i++;
    }

    return res;
}

// Json StrategyMarketMaker::get_open_orders()
// {
//     std::unordered_map<PAState, StrategyMarketMakerState*>* strategy_states = get_strategy_states();
//     PAState state = m_current_state.object.state;

//     // Run get_open_orders() method of new state
//     if ((*strategy_states).find(state) != (*strategy_states).end())
//     {
//         return (*strategy_states)[state]->get_open_orders();
//     }

//     return Json::create_array();
// }