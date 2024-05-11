#ifndef DATA_FEED_BINANCE_FUTURES_AGG_TRADE_H
#define DATA_FEED_BINANCE_FUTURES_AGG_TRADE_H

//#include <vector>
#include <functional>
#include <data_feed/data_feed_binance_spot/data_feed_binance.h>

using namespace std;

class DataFeedBinanceFuturesAggTrade : public DataFeedBinance
{
public:
    DataFeedBinanceFuturesAggTrade(const string& symbol);
    ~DataFeedBinanceFuturesAggTrade();

    virtual void init();
    void set_call_back(function<void(const string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const string& buffer, Json& data);

private:
    string m_symbol;
    function<void(const string& symbol, Json& payload)> m_on_callback = nullptr;
};

#endif //DATA_FEED_BINANCE_FUTURES_AGG_TRADE_H