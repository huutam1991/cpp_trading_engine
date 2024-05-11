#include <storage_source/binance/storage_source_binance_testnet.h>

void StorageSourceBinanceTestnet::init_endpoint()
{
    // url
    /*m_url = BINANCE_TESTNET_URL;
    m_port = BINANCE_TESTNET_PORT;

    // websocket
    m_ws_url = BINANCE_TESTNET_WS_URL;
    m_ws_port = BINANCE_TESTNET_WS_PORT;*/
}

const char* StorageSourceBinanceTestnet::get_db_name() const
{
    return BINANCE_TESTNET_DB_SOURCE_NAME;
}