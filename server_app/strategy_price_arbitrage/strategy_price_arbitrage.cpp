

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

    // State

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
        std::unique_lock lock(m_strategy_mutex);

        // ADD_LOG("symbol: " << symbol << ", price: " << price);

        // Can miss some price update
        if (m_has_data_update.is_value_set() == false)
        {
            m_state_data_queue.push(PriceUpdate{std::move(symbol), price});

            // Inform has data update
            m_has_data_update.set_value(true);
        }
    });
    m_gateway->subscribe_symbol({m_config.symbol_1, m_config.symbol_2});

    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order& order)
    {
        std::unique_lock lock(m_strategy_mutex);

        m_state_data_queue.push(order);

        // Inform has data update
        m_has_data_update.set_value(true);
    });

    //
    m_gateway->check_remove_canceled_orders(m_config.symbol_1);

    if (m_is_run_update == false)
    {
        m_update_task = update();
        m_update_task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::STRATEGY));

        m_is_run_update = true;
    }
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

TaskVoid StrategyPriceArbitrage::update()
{
    std::unordered_map<PAState, StrategyPriceArbitrageState*>* strategy_states = get_strategy_states();
    PAState current_state = PAState::PA_STOP;

    while (true)
    {
        // Dont do update when strategy is init
        if (m_is_init == true)
        {
            co_await Timer::sleep_for(2000);
            continue;
        }

        co_await wait_new_data_update();

        while (m_state_data_queue.size() > 0)
        {
            StrategyPriceArbitrageData data;
            {
                std::unique_lock lock(m_strategy_mutex);

                data = m_state_data_queue.front();
                m_state_data_queue.pop();
            }

            PAState state = m_current_state.object.state;

            // Check change state
            if (current_state != state)
            {
                // Run end() method of current state
                if ((*strategy_states).find(current_state) != (*strategy_states).end())
                {
                    (*strategy_states)[current_state]->end();
                }

                // Run begin() method of new state
                if ((*strategy_states).find(state) != (*strategy_states).end())
                {
                    (*strategy_states)[state]->begin();
                }
            }

            // Update [current_status]
            current_state = state;

            co_await (*strategy_states)[current_state]->run(std::move(data));
        }

    }
}

Future<bool> StrategyPriceArbitrage::wait_new_data_update()
{
    return Future<bool>([this](Future<bool>::FutureValue value)
    {
        m_has_data_update = value;
    });
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