#pragma once

#include <memory>
#include <string>
#include <functional>

#include <instrument/instrument.h>
#include <network/https_client_websocket/https_client_websocket.h>

class OrderBookWebsocket
{
public:
    OrderBookWebsocket(const Instrument* instrument, size_t depth_level, EpollBase* event_base, std::function<void(std::string)> on_order_book_ws);
    ~OrderBookWebsocket()
    {
        // m_websocket->close();
    }

private:
    const Instrument* m_instrument;
    size_t m_depth_level;
    EpollBase* m_event_base;
    std::function<void(std::string)> m_on_order_book_ws;

    std::shared_ptr<HttpsClientWebsocket> m_websocket;

    void start();
};