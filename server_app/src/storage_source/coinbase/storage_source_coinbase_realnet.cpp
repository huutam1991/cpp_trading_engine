#include <storage_source/coinbase/storage_source_coinbase_realnet.h>

void StorageSourceCoinbaseRealnet::init_endpoint()
{
    // url
    // m_url = COINBASE_REALNET_URL;
    // m_port = COINBASE_REALNET_PORT;

    // // websocket
    // m_ws_url = COINBASE_REALNET_WS_URL;
    // m_ws_port = COINBASE_REALNET_WS_PORT;
}

const char* StorageSourceCoinbaseRealnet::get_db_name() const
{
    return COINBASE_REALNET_DB_SOURCE_NAME;
}