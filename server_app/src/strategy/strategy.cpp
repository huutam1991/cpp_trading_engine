

#include <strategy/strategy.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <app_constants.h>

void Strategy::init()
{
    SimpleGuard g(&m_is_init);

    // Load current strategy info
    Json config = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "config")
        .find_any();

    m_symbol = std::string(config["symbol"]);
    m_volumn = config["volumn"];
    m_move_price = config["move_price"];
    m_sell_buy_ratio = config["sell_buy_ratio"];
    m_is_running = config["is_running"];

    // Log config
    ADD_LOG("Strategy config:");
    ADD_LOG("- symbol: " << m_symbol);
    ADD_LOG("- volumn: " << m_volumn);
    ADD_LOG("- move_price: " << m_move_price);
    ADD_LOG("- sell_buy_ratio: " << m_sell_buy_ratio);
    ADD_LOG("- is_running: " << m_is_running);

    // Load checkpoints
    m_checkpoints = std::make_shared<CheckPoints>(m_symbol, m_volumn, m_move_price, m_sell_buy_ratio);

    // Add price callback + subscribe to symbol
    auto gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    gateway->register_price_update([this](double price)
    {
        this->update(price);
    });
    gateway->subscribe_symbol(m_symbol);
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

    if (current_checkpoint.is_null() == false)
    {
        current_checkpoint["is_current_checkpoint"] = true;
    }
    else
    {
        DataModel new_checkpoint = m_checkpoints->create_checkpoint_data_model(m_current_price);
        new_checkpoint["is_current_checkpoint"] = true;
    }
}

void Strategy::stop()
{
    DataModel current_checkpoint = m_checkpoints->get_current_checkpoint();
    ADD_LOG("current_checkpoint: " << current_checkpoint);

    if (current_checkpoint.is_null() == false)
    {
        current_checkpoint["is_current_checkpoint"] = false;
    }
}

void Strategy::update(double price)
{
    // Dont do update when strategy is init
    if (m_is_init == true) return;

    m_current_price = price;
}

double Strategy::get_current_price()
{
    return m_current_price;
}