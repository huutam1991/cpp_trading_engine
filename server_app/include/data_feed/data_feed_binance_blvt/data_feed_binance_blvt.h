#ifndef DATA_FEED_BINANCE_BLVT_H
#define DATA_FEED_BINANCE_BLVT_H

#include <mutex>
#include <data_feed/data_feed.h>

class DataFeedBinanceBLVT : public DataFeed
{
public:
    DataFeedBinanceBLVT();
    virtual ~DataFeedBinanceBLVT() {}

    virtual void init() {};

protected:
    std::string m_url;
    std::string m_port;
    std::string m_binance_ws_url;
    std::string m_binance_ws_port;
    size_t m_id = 0;

    virtual bool standardize_data(const std::string&, Json&) {return false;}
    //void check_ws_url_base_on_back_testing();

    static std::mutex m_df_bs_mutex;
    static size_t get_stream_id_count();
};

#endif //DATA_FEED_BINANCE_BLVT_H
