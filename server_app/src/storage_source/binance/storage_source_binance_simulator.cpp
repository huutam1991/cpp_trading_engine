#include <storage_source/binance/storage_source_binance_simulator.h>

void StorageSourceBinanceSimulator::init_endpoint()
{
    // url
    //m_url = BINANCE_SIMULATOR_URL;
    //m_port = BINANCE_SIMULATOR_PORT;

    // websocket
    //m_ws_url = BINANCE_SIMULATOR_WS_URL;
    //m_ws_port = BINANCE_SIMULATOR_WS_PORT;

    if (const char* uri_env = std::getenv("BINANCE_SIMULATOR_URL"))
    {
        //m_url = uri_env;
        //m_ws_url = uri_env;
    }

    // Fake API key + API secret
    init_simulate_api_key_and_api_secret();
}

const char* StorageSourceBinanceSimulator::get_db_name() const
{
    return BINANCE_SIMULATOR_DB_SOURCE_NAME;
}

void StorageSourceBinanceSimulator::init_simulate_api_key_and_api_secret()
{
    // Insert API Key + API Secret to DB if not exist
    /*MongoQuery query = MongoDB::instance()
        .set_db_and_collection(get_db_name(), "info");

    query.delete_one("user_id", m_user_id);
    query.insert_one({
        {"api_key", m_user_id + "-binance_simulator_api_key"},
        {"api_secret", "binance_simulator_api_secret"},
        {"user_id", m_user_id},
    });*/
}