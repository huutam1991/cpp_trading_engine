#ifndef DATA_FEED_BINANCE_FUTURES_DEPTH_H
#define DATA_FEED_BINANCE_FUTURES_DEPTH_H

#include <functional>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures.h>

/*  Top bids and asks, Valid are 5, 10, or 20.
    Stream Names: <symbol>@depth<levels> OR <symbol>@depth<levels>@500ms OR <symbol>@depth<levels>@100ms.
    Update Speed: 250ms, 500ms or 100ms
*/
class DataFeedBinanceFuturesDepth : public DataFeedBinanceFutures
{
public:
    DataFeedBinanceFuturesDepth(const std::string& symbol);
    ~DataFeedBinanceFuturesDepth();

    virtual void init();
    void set_call_back(std::function<void(const std::string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& data, Json& depth);

private:
    std::string m_symbol;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;
};

#endif //DATA_FEED_BINANCE_FUTURES_DEPTH_H