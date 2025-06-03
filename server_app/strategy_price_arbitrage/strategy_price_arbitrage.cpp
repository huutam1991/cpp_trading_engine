

#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <time/timer.h>
#include <app_constants.h>
#include <app_utils/app_utils.h>

// StrategyState
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_stop.h>

std::unordered_map<PAState, StrategyPriceArbitrageState*>* StrategyPriceArbitrage::get_strategy_states()
{
    static std::unordered_map<PAState, StrategyPriceArbitrageState*> m_strategy_states;

    // Init StrategyState by name
    if (m_strategy_states.size() == 0)
    {
        std::shared_ptr<Gateway>& gateway = StrategyPriceArbitrage::instance().m_gateway;

        m_strategy_states[PAState::PA_RUN] = new StrategyPriceArbitrageStateRun(gateway, StrategyPriceArbitrage::instance().m_config);
        m_strategy_states[PAState::PA_STOP] = new StrategyPriceArbitrageStateStop(gateway, StrategyPriceArbitrage::instance().m_config);
    }

    return &m_strategy_states;
}

void StrategyPriceArbitrage::init()
{
    PriceArbitrageSimpleGuard g(m_is_init);

    // Get [m_previous_state]
    m_previous_state = m_current_state.object.state;

    // Load current strategy info
    DataModel config = DataModel::load_single_data_model(STRATEGY_DB_NAME, "price_arbitrage_config");

    // If there's no config data available, add default config
    if (config.get_data().has_field("symbol_1") == false)
    {
        config = {
            {"symbol_1", "BTCUSDT"},
            {"symbol_2", "ETHBTC"},
            {"symbol_3", "ETHUSDT"},
            {"buy_volumn", (long)300},
            {"buy_at_lower_price", (long)200},
            {"price_delta", (long)10},
            {"too_low_price_delta", (long)90},
            {"too_high_price_delta", (long)30},
            {"is_running", false}
        };
    }

    m_config.symbol_1 = std::string(config["symbol_1"]);
    m_config.symbol_2 = std::string(config["symbol_2"]);
    m_config.symbol_3 = std::string(config["symbol_3"]);
    m_config.buy_volumn = (double)config["buy_volumn"];
    m_config.buy_at_lower_price = (double)config["buy_at_lower_price"];
    m_config.price_delta = (double)config["price_delta"];
    m_config.too_low_price_delta = (double)config["too_low_price_delta"];
    m_config.too_high_price_delta = (double)config["too_high_price_delta"];
    m_config.is_running = (bool)config["is_running"];

    // Log config
    ADD_LOG("StrategyPriceArbitrage config:");
    ADD_LOG("- symbol_1: " << m_config.symbol_1);
    ADD_LOG("- symbol_2: " << m_config.symbol_2);
    ADD_LOG("- symbol_3: " << m_config.symbol_3);
    ADD_LOG("- buy_volumn: " << m_config.buy_volumn);
    ADD_LOG("- buy_at_lower_price: " << m_config.buy_at_lower_price);
    ADD_LOG("- price_delta: " << m_config.price_delta);
    ADD_LOG("- too_low_price_delta: " << m_config.too_low_price_delta);
    ADD_LOG("- too_high_price_delta: " << m_config.too_high_price_delta);
    ADD_LOG("- is_running: " << m_config.is_running);

    // Add price callback + subscribe to symbol
    m_gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    m_gateway->register_price_update([this](std::string symbol, double price)
    {
        update(PriceUpdate{std::move(symbol), price}).start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::STRATEGY));
    });
    m_gateway->subscribe_symbol({m_config.symbol_1, m_config.symbol_2});

    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order& order)
    {
        update(order).start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::STRATEGY));
    });

    //
    m_gateway->check_remove_canceled_orders(m_config.symbol_1);
}

void StrategyPriceArbitrage::on_config_change()
{
    // Re-init config
    init();

    // Check start-stop
    if (m_config.is_running)
    {
        run();
    }
    else
    {
        stop();
    }
}

void StrategyPriceArbitrage::run()
{
    m_current_state = PAStateData{PAState::PA_RUN};
}

void StrategyPriceArbitrage::stop()
{
    m_current_state = PAStateData{PAState::PA_STOP};
}

TaskVoid StrategyPriceArbitrage::update(StrategyPriceArbitrageData data)
{
    // Dont do update when strategy is init
    if (m_is_init == true)
    {
        co_return;
    }

    std::unordered_map<PAState, StrategyPriceArbitrageState*>* strategy_states = get_strategy_states();
    PAState current_state = m_current_state.object.state;

    // Check change state
    if (m_previous_state != current_state)
    {
        // Run end() method of m_previous_state
        if ((*strategy_states).find(m_previous_state) != (*strategy_states).end())
        {
            (*strategy_states)[m_previous_state]->end();
        }

        // Run begin() method of new state
        if ((*strategy_states).find(current_state) != (*strategy_states).end())
        {
            (*strategy_states)[current_state]->begin();
        }
    }

    // Update [current_status]
    m_previous_state = current_state;

    co_await (*strategy_states)[current_state]->run(std::move(data));

    co_return;
}

Json StrategyPriceArbitrage::get_orders_chain()
{
    Json info = Json::create_array();

    Json orders = MongoDB::instance()
        .set_db_and_collection("order", "order_list")
        .find_many();

    orders.for_each([&info](Json& order)
    {
        if (order["status"] == "FILLED")
        {
            order.remove_field("_id");
            info.push_back(order);
        }
    });

    info.sort([](Json& a, Json& b)
    {
        return (size_t)a["order_id"] < (size_t)b["order_id"];
    });

    return info;
}

Json StrategyPriceArbitrage::get_open_orders()
{
    std::unordered_map<PAState, StrategyPriceArbitrageState*>* strategy_states = get_strategy_states();
    PAState state = m_current_state.object.state;

    // Run get_open_orders() method of new state
    if ((*strategy_states).find(state) != (*strategy_states).end())
    {
        return (*strategy_states)[state]->get_open_orders();
    }

    return Json::create_array();
}