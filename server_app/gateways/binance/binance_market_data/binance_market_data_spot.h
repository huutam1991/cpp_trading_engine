#pragma once

#include <functional>
#include <unordered_map>

#include <network/https_client_websocket/https_client_websocket.h>
#include <json/json.h>
#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>

#include <instrument/instrument.h>

class BinanceMarketDataSpot
{
public:
    BinanceMarketDataSpot(const std::string& url, const std::string& port);
    ~BinanceMarketDataSpot();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start();
    void start_websocket(const Instrument* instrument);
    void subscribe_instruments(std::vector<const Instrument*> instruments);

protected:
    virtual bool standardize_data(const std::string& buffer, Json& data);

private:
    std::string m_url;
    std::string m_port;
    std::vector<const Instrument*> m_instruments;

    EpollBase* m_event_base = nullptr;

    std::unordered_map<const Instrument*, std::shared_ptr<HttpsClientWebsocket>> m_websockets;

    size_t get_stream_id_count();
};
