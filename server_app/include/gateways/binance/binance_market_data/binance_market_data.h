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
    void subscribe_symbol(std::vector<std::string> symbols, std::function<void(const std::string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& buffer, Json& data);

private:
    std::string m_url;
    std::string m_port;
    std::vector<std::string> m_symbols;

    EventBase* m_event_base = nullptr;

    std::vector<std::shared_ptr<WebsocketClientAsync>> m_websockets;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;

    size_t get_stream_id_count();
};

#endif //BINANCE_MARKET_DATA_H