#ifndef DATA_FEED_BINANCE_TICKER_H
#define DATA_FEED_BINANCE_TICKER_H

//#include <vector>
#include <functional>
//#include <app_constants.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance.h>

class DataFeedBinanceBookTicker : public DataFeedBinance
{
public:
    DataFeedBinanceBookTicker(const std::string& symbol);
    ~DataFeedBinanceBookTicker();

    virtual void init();
    void set_call_back(std::function<void(const std::string& symbol, Json& ticker)> call_back);

protected:
    virtual bool standardize_data(const std::string& data, Json& ticker);

private:
    std::string m_symbol;
    std::function<void(const std::string& symbol, Json& ticker)> m_on_callback = nullptr;
};

#endif //DATA_FEED_BINANCE_TICKER_H