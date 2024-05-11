#ifndef DATA_FEED_BINANCE_FUTURES_SYMBOL_BOOK_TICKER_H
#define DATA_FEED_BINANCE_FUTURES_SYMBOL_BOOK_TICKER_H

//#include <vector>
#include <functional>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures.h>

/*  Pushes any update to the best bid or ask's price or quantity in real-time for a specified symbol.
    Stream Name: <symbol>@bookTicker
    Update Speed: Real-time
*/
class DataFeedBinanceFuturesBookTicker : public DataFeedBinanceFutures
{
public:
    DataFeedBinanceFuturesBookTicker(const std::string& symbol);
    ~DataFeedBinanceFuturesBookTicker();

    virtual void init();
    void set_call_back(std::function<void(const std::string& symbol, Json& ticker)> call_back);

protected:
    virtual bool standardize_data(const std::string& data, Json& ticker);

private:
    std::string m_symbol;
    std::function<void(const std::string& symbol, Json& ticker)> m_on_callback = nullptr;
};

#endif //DATA_FEED_BINANCE_FUTURES_SYMBOL_BOOK_TICKER_H