

#include <strategy/strategy.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <app_constants.h>

// StrategyState
#include <strategy/strategy_state/strategy_state_start.h>
#include <strategy/strategy_state/strategy_state_stop.h>

std::unordered_map<std::string, StrategyState*>* Strategy::get_strategy_states()
{
    static std::unordered_map<std::string, StrategyState*> m_strategy_states;

    // Init StrategyState by name
    if (m_strategy_states.size() == 0)
    {
        CheckPoints* checkpoints = Strategy::instance().m_checkpoints.get();
        Gateway* gateway = Strategy::instance().m_gateway.get();

        m_strategy_states["START"] = new StrategyStateStart(gateway, checkpoints);
        m_strategy_states["STOP"] = new StrategyStateStop(gateway, checkpoints);
    }

    return &m_strategy_states;
}

void Strategy::init()
{
    SimpleGuard g(m_is_init);

    // Load current strategy info
    std::vector<DataModel> configs = DataModel::get_data_model_list(STRATEGY_DB_NAME, "config");
    if (configs.size() > 0)
    {
        DataModel config = configs[0];

        m_symbol = std::string(config["symbol"]);
        m_buy_volumn = config["buy_volumn"];
        m_move_price = config["move_price"];
        m_sell_buy_ratio = config["sell_buy_ratio"];
        m_is_running = config["is_running"];

        // Log config
        ADD_LOG("Strategy config:");
        ADD_LOG("- symbol: " << m_symbol);
        ADD_LOG("- buy_volumn: " << m_buy_volumn);
        ADD_LOG("- move_price: " << m_move_price);
        ADD_LOG("- sell_buy_ratio: " << m_sell_buy_ratio);
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
    DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();

    DataModel status = StrategyState::get_state_status();
    status["status"] = "START";

    // if (current_checkpoint.is_null() == false)
    // {
    //     current_checkpoint["is_current_checkpoint"] = true;
    // }
    // else
    // {
    //     DataModel new_checkpoint = m_checkpoints->create_checkpoint_data_model(m_current_price);
    //     new_checkpoint["is_current_checkpoint"] = true;
    // }
}

void Strategy::stop()
{
    DataModel status = StrategyState::get_state_status();
    status["status"] = "STOP";

    // DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();
    // ADD_LOG("current_checkpoint: " << current_checkpoint);

    // if (current_checkpoint.is_null() == false)
    // {
    //     current_checkpoint["is_current_checkpoint"] = false;
    // }
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