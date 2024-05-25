#include <strategy/strategy.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <app_constants.h>

void Strategy::init()
{
    // Load current strategy info
    Json config = MongoDB::instance()
        .set_db_and_collection(STRATEGY_DB_NAME, "config")
        .find_any();

    m_symbol = std::string(config["symbol"]);
    m_volumn = config["volumn"];
    m_move_price = config["move_price"];
    m_is_running = config["is_running"];

    // Log config
    ADD_LOG("Strategy config:");
    ADD_LOG("- symbol: " << m_symbol);
    ADD_LOG("- volumn: " << m_volumn);
    ADD_LOG("- move_price: " << m_move_price);
    ADD_LOG("- is_running: " << m_is_running);
}