

#include <strategy/strategy.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <app_constants.h>

// StrategyState
#include <strategy/strategy_state/strategy_state_start.h>
#include <strategy/strategy_state/strategy_state_placing.h>
#include <strategy/strategy_state/strategy_state_monitoring.h>
#include <strategy/strategy_state/strategy_state_stop.h>

std::unordered_map<std::string, StrategyState*>* Strategy::get_strategy_states()
{
    static std::unordered_map<std::string, StrategyState*> m_strategy_states;

    // Init StrategyState by name
    if (m_strategy_states.size() == 0)
    {
        std::shared_ptr<Gateway>& gateway = Strategy::instance().m_gateway;
        std::shared_ptr<CheckPoints>& check_point = Strategy::instance().m_checkpoints;

        m_strategy_states["START"] = new StrategyStateStart(gateway, check_point);
        m_strategy_states["PLACING"] = new StrategyStatePlacing(gateway, check_point);
        m_strategy_states["MONITORING"] = new StrategyStateMonitoring(gateway, check_point);
        m_strategy_states["STOP"] = new StrategyStateStop(gateway, check_point);
    }

    return &m_strategy_states;
}

void Strategy::init()
{
    SimpleGuard g(m_is_init);

    // Load current strategy info
    DataModel config = DataModel::get_single_data_model(STRATEGY_DB_NAME, "config");

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

    // Load checkpoints
    m_checkpoints = std::make_shared<CheckPoints>(m_symbol, m_buy_volumn, m_move_price, m_sell_buy_ratio);

    // Add price callback + subscribe to symbol
    m_gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    m_gateway->register_price_update([this](double price)
    {
        this->update(price);
    });
    m_gateway->subscribe_symbol(m_symbol);
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
        stop();
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

void Strategy::update(double price)
{
    // Dont do update when strategy is init
    if (m_is_init == true) return;

    m_current_price = price;

    std::unordered_map<std::string, StrategyState*>* strategy_states = get_strategy_states();
    std::string status = StrategyState::get_state_status()["status"];

    (*strategy_states)[status]->run(price);
}

double Strategy::get_current_price()
{
    return m_current_price;
}

double Strategy::get_total_profit()
{
    return m_checkpoints->get_total_profit();
}