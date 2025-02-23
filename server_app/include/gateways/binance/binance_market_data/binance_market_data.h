#ifndef BINANCE_MARKET_DATA_H
#define BINANCE_MARKET_DATA_H

#include <functional>
#include <websocket/websocket_client.h>
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

    std::shared_ptr<WebsocketClient> m_websocket;
    std::function<void(const std::string& symbol, Json& payload)> m_on_callback = nullptr;

    size_t get_stream_id_count();

    // For checking websocket stream is stop
    size_t m_websocket_data_counter = 0;
    size_t m_schedule_task_id = 0;
    void add_timer_to_check_websocket_stream_is_stop(size_t period);
    void del_timer_to_check_websocket_stream_is_stop();
};

#endif //BINANCE_MARKET_DATA_H