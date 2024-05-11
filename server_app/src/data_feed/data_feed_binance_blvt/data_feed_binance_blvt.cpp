//#include <json/json.h>
//#include <app_constants.h>
#include <price_manager/price_manager.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt.h>
//#include <back_testing/back_testing.h>

std::mutex DataFeedBinanceBLVT::m_df_bs_mutex;

DataFeedBinanceBLVT::DataFeedBinanceBLVT() : DataFeed(BINANCE_COMMON)
{
    m_url = BINANCE_BLVT_URL;
    m_port = BINANCE_BLVT_PORT;
    m_binance_ws_url  = BINANCE_BLVT_WS_URL;
    m_binance_ws_port = BINANCE_BLVT_WS_PORT;
}

size_t DataFeedBinanceBLVT::get_stream_id_count()
{
    static int stream_id_count = 0;
    std::unique_lock lock(m_df_bs_mutex);

    return ++stream_id_count;
}
