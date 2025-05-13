#ifndef COINBASE_MARKET_DATA_H
#define COINBASE_MARKET_DATA_H

#include <functional>
#include <unordered_map>

#include <websocket/websocket_client_async.h>
#include <json/json.h>

class CoinbaseMarketData
{
public:
    CoinbaseMarketData(const std::string& url, const std::string& port);
    ~CoinbaseMarketData();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start();
    void start_websocket(std::string symbol);
    void subscribe_symbol(std::vector<std::string> symbols, std::function<void(const std::string& symbol, Json& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& buffer, Json& data);

private:
    std::string m_url;
    std::string m_port;
    std::vector<std::string> m_symbols;

    EventBase* m_event_base = nullptr;

    std::unordered_map<std::string, std::shared_ptr<WebsocketClientAsync>> m_websockets;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;

    size_t get_stream_id_count();
};

#endif //COINBASE_MARKET_DATA_H