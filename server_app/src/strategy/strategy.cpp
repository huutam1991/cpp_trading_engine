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

    ADD_LOG("Strategy config: " << config);
}