#ifndef BINANCE_MARKET_DATA_H
#define BINANCE_MARKET_DATA_H

#include <functional>
#include <websocket/websocket_client_async.h>
#include <json/json.h>

class BinanceMarketData
{
public:
    BinanceMarketData(const std::string& url, const std::string& port);
    ~BinanceMarketData();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start();
    void subscribe_symbol(const std::string& symbol, std::function<void(const std::string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& buffer, Json& data);

private:
    std::string m_url;
    std::string m_port;
    std::string m_symbol;

    EventBase* m_event_base = nullptr;

    std::shared_ptr<WebsocketClientAsync> m_websocket;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;

    size_t get_stream_id_count();
};

#endif //BINANCE_MARKET_DATA_H