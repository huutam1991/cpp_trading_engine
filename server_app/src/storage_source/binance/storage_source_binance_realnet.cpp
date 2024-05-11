#include <storage_source/binance/storage_source_binance_realnet.h>

void StorageSourceBinanceRealnet::init_endpoint()
{
    // url
    /*m_url = BINANCE_SPOT_URL;
    m_port = BINANCE_SPOT_PORT;

    // websocket
    m_ws_url = BINANCE_SPOT_WS_URL;
    m_ws_port = BINANCE_SPOT_WS_PORT;*/
}

const char* StorageSourceBinanceRealnet::get_db_name() const
{
    return BINANCE_SPOT_DB_SOURCE_NAME;
}