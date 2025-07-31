#pragma once

#include <functional>
#include <unordered_map>

#include <websocket/websocket_client_async.h>
#include <c_json/json.h>

#include <instrument/instrument.h>

class BinanceMarketDataSpot
{
public:
    BinanceMarketDataSpot(const std::string& url, const std::string& port);
    ~BinanceMarketDataSpot();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start();
    void start_websocket(const Instrument* instrument);
    void subscribe_instruments(std::vector<const Instrument*> instruments, std::function<void(const Instrument* symbol, JsonNew& payload)> call_back);

protected:
    virtual bool standardize_data(const std::string& buffer, JsonNew& data);

private:
    std::string m_url;
    std::string m_port;
    std::vector<const Instrument*> m_instruments;

    EventBase* m_event_base = nullptr;

    std::unordered_map<const Instrument*, std::shared_ptr<WebsocketClientAsync>> m_websockets;
    std::function<void(const Instrument* symbol, JsonNew& payload)> m_on_callback = nullptr;

    size_t get_stream_id_count();
};
