#ifndef BINANCE_MARKET_DATA_H
#define BINANCE_MARKET_DATA_H

#include <functional>
#include <app_constants.h>
#include <websocket/websocket_client.h>
#include <json/json.h>

class BinanceMarketData
{
public:
    BinanceMarketData(const std::string& symbol);
    ~BinanceMarketData();

    virtual void start();
    void set_call_back(std::function<void(const std::string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& buffer, Json& data);

private:
    std::shared_ptr<WebsocketClient> m_websocket;
    std::string m_symbol;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;

    size_t get_stream_id_count();
};

#endif //BINANCE_MARKET_DATA_H