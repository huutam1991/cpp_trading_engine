#ifndef DATA_FEED_BINANCE_DEPTH_H
#define DATA_FEED_BINANCE_DEPTH_H

//#include <vector>
#include <functional>
#include <data_feed/data_feed_binance_spot/data_feed_binance.h>

class DataFeedBinanceDepth : public DataFeedBinance
{
public:
    DataFeedBinanceDepth(const std::string& symbol);
    ~DataFeedBinanceDepth();

    virtual void init();
    void set_call_back(std::function<void(const std::string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& buffer, Json& data);

private:
    std::string m_symbol;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;
};

#endif //DATA_FEED_BINANCE_DEPTH_H