#include <storage_source/coinbase/storage_source_coinbase_testnet.h>

void StorageSourceCoinbaseTestnet::init_endpoint()
{
    // url
    // m_url = COINBASE_TESTNET_URL;
    // m_port = COINBASE_TESTNET_PORT;

    // // websocket
    // m_ws_url = COINBASE_TESTNET_WS_URL;
    // m_ws_port = COINBASE_TESTNET_WS_PORT;
}

const char* StorageSourceCoinbaseTestnet::get_db_name() const
{
    return COINBASE_TESTNET_DB_SOURCE_NAME;
}