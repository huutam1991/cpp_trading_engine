

#include <strategy/strategy.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <app_constants.h>
#include <app_utils.h>

// StrategyState
#include <strategy/strategy_state/strategy_state_start.h>
#include <strategy/strategy_state/strategy_state_placing.h>
#include <strategy/strategy_state/strategy_state_monitoring.h>
#include <strategy/strategy_state/strategy_state_stop.h>
#include <strategy/strategy_state/strategy_state_close_all_positions.h>

std::unordered_map<std::string, StrategyState*>* Strategy::get_strategy_states()
{
    static std::unordered_map<std::string, StrategyState*> m_strategy_states;

    // Init StrategyState by name
    if (m_strategy_states.size() == 0)
    {
        std::shared_ptr<Gateway>& gateway = Strategy::instance().m_gateway;
        std::shared_ptr<CheckPointList>& check_point = Strategy::instance().m_checkpoints;

        m_strategy_states["START"] = new StrategyStateStart(gateway, check_point);
        m_strategy_states["PLACING"] = new StrategyStatePlacing(gateway, check_point);
        m_strategy_states["MONITORING"] = new StrategyStateMonitoring(gateway, check_point);
        m_strategy_states["STOP"] = new StrategyStateStop(gateway, check_point);
        m_strategy_states["CLOSE_ALL_POSITIONS"] = new StrategyStateCloseAllPositions(gateway, check_point);
    }

    return &m_strategy_states;
}

void Strategy::init()
{
    SimpleGuard g(m_is_init);

    // Load current strategy info
    DataModel config = DataModel::load_single_data_model(STRATEGY_DB_NAME, "config");

    // If there's no config data available, add default config
    if (config.get_data().has_field("symbol") == false)
    {
        config = {
            {"symbol", "ETHUSDT"},
            {"buy_volumn", (long)30},
            {"move_price", (long)307},
            {"sell_buy_ratio", 0.4},
            {"is_running", false}
        };
    }

    m_symbol = std::string(config["symbol"]);
    m_buy_volumn = config["buy_volumn"];
    m_move_price = config["move_price"];
    m_sell_buy_ratio = config["sell_buy_ratio"];
    m_is_running = config["is_running"];
    m_is_close_all_positions = config["is_close_all_positions"];

    // Get [placing_price]
    double placing_price = config["placing_price"];
    StrategyState::set_placing_price(placing_price);

    // Log config
    ADD_LOG("Strategy config:");
    ADD_LOG("- symbol: " << m_symbol);
    ADD_LOG("- buy_volumn: " << m_buy_volumn);
    ADD_LOG("- move_price: " << m_move_price);
    ADD_LOG("- sell_buy_ratio: " << m_sell_buy_ratio);
    ADD_LOG("- placing_price: " << placing_price);
    ADD_LOG("- is_running: " << m_is_running);
    ADD_LOG("- is_close_all_positions: " << m_is_close_all_positions);

    // Load checkpoints
    m_checkpoints = std::make_shared<CheckPointList>(m_symbol, m_buy_volumn, m_move_price, m_sell_buy_ratio);

    // Add price callback + subscribe to symbol
    m_gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    m_gateway->register_price_update([this](double price)
    {
        std::unique_lock lock(m_strategy_mutex);

        // Can miss some price update
        if (m_has_data_update.is_value_set() == false)
        {
            m_state_data_queue.push(price);
            m_current_price = price;

            // Inform has data update
            m_has_data_update.set_value(true);
        }
    });
    m_gateway->subscribe_symbol(m_symbol);

    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order& order)
    {
        std::unique_lock lock(m_strategy_mutex);

        m_state_data_queue.push(order);

        // Inform has data update
        m_has_data_update.set_value(true);
    });

    // Destroy old task
    m_update_task.destroy();
    // Create new task
    m_update_task = update();
    m_update_task.start_running_on(EventBaseManager::instance().get_event_base_by_id(EventBaseID::STRATEGY));
}

void Strategy::on_config_change()
{
    // Re-init config
    init();

    // Check start-stop
    if (m_is_running)
    {
        start();
    }
    else
    {
        // Only close all positions when [m_is_running] == false and [m_is_close_all_positions] == true
        if (m_is_close_all_positions == true)
        {
            close_all_positions();
        }
        else
        {
            stop();
        }
    }
}

void Strategy::start()
{
    StrategyState::set_state_status("START");
}

void Strategy::stop()
{
    StrategyState::set_state_status("STOP");
}

void Strategy::close_all_positions() {
    StrategyState::set_state_status("CLOSE_ALL_POSITIONS");
}

TaskVoid Strategy::update()
{
    while (true)
    {
        // Dont do update when strategy is init
        if (m_is_init == true)
        {
            co_await Future<size_t>::sleep_for_seconds(2);
            continue;
        }

        co_await wait_new_data_update();

        while (m_state_data_queue.size() > 0)
        {
            StateData data;
            {
                std::unique_lock lock(m_strategy_mutex);

                data = m_state_data_queue.front();
                m_state_data_queue.pop();
            }

            std::unordered_map<std::string, StrategyState*>* strategy_states = get_strategy_states();
            std::string status = StrategyState::get_state_status()["status"];

            co_await (*strategy_states)[status]->run(std::move(data));
        }

    }
}


Future<bool> Strategy::wait_new_data_update()
{
    return Future<bool>([this](Future<bool>::FutureValue value)
    {
        m_has_data_update = value;
    });
}

double Strategy::get_current_price()
{
    return m_current_price;
}

DataModel Strategy::get_checkpoint_by_price(double price)
{
    return m_checkpoints->get_checkpoint_by_price(price);
}

Json Strategy::get_current_info()
{
    double market_price = get_current_price();
    Json info = m_checkpoints->get_current_info();
    info["current_price"] = market_price;

    double next_price = info["neighbor_checkpoints"][0]["next_price"];
    double curr_price = info["neighbor_checkpoints"][1]["curr_price"];
    double prev_price = info["neighbor_checkpoints"][2]["prev_price"];

    if (abs(market_price - curr_price) < 1.0)
    {
        info["neighbor_checkpoints"][1]["price_distance"] = market_price - curr_price;
    }
    else if (next_price - market_price < market_price - prev_price)
    {
        info["neighbor_checkpoints"][0]["price_distance"] = next_price - market_price;
        info["neighbor_checkpoints"][0]["to_curr_price"] = curr_price - market_price;
    }
    else
    {
        info["neighbor_checkpoints"][2]["price_distance"] = market_price - prev_price;
        info["neighbor_checkpoints"][2]["to_curr_price"] = market_price - curr_price;
    }

    return info;
}