#ifndef DATA_FEED_BINANCE_BLVT_CANDLESTICK_H
#define DATA_FEED_BINANCE_BLVT_CANDLESTICK_H

//#include <vector>
#include <functional>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt.h>

using namespace std;

class DataFeedBinanceBLVTCandlestick : public DataFeedBinanceBLVT
{
public:
    DataFeedBinanceBLVTCandlestick(const string& symbol);
    ~DataFeedBinanceBLVTCandlestick();

    virtual void init();
    void set_call_back(function<void(const string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const string& data, Json& depth);

private:
    string m_symbol;
    function<void(const string& symbol, Json& payload)> m_on_callback = nullptr;
};

#endif //DATA_FEED_BINANCE_BLVT_CANDLESTICK_H