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
    void subscribe_instrument(const Instrument* instrument) { m_instruments.push_back(instrument); }
    void unsubscribe_instrument(const Instrument* instrument)
    {
        m_instruments.erase(std::remove(m_instruments.begin(), m_instruments.end(), instrument), m_instruments.end());
    }

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
