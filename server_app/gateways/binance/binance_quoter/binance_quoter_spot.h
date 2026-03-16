#pragma once

#include <network/https_client_websocket/https_client_websocket.h>
#include <coroutine/event_base_manager.h>
#include <gateways/binance/binance_quoter/binance_quoter.h>

class BinanceQuoterSpot : public BinanceQuoter
{
private:
    std::string m_url = BINANCE_SPOT_URL;
    std::string m_port = BINANCE_SPOT_PORT;
    std::string m_ws_url = BINANCE_SPOT_WS_URL;
    std::string m_ws_port = BINANCE_SPOT_WS_PORT;

    EpollBase* m_epoll_base = nullptr;

    // Websocket to get order data
    std::shared_ptr<HttpsClientWebsocket> m_websocket;
    std::string m_listen_key;
    void init_websocket();
    Task<std::string> get_listen_key();
    Task<void> keep_listen_key();

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    BinanceQuoterSpot(const std::string& key);
    ~BinanceQuoterSpot();

    virtual Task<Json> get_open_orders(std::string symbol) override;
    virtual Task<void> cancel_all(std::string symbol) override;
    virtual Task<Json> cancel(Order order) override;
    virtual Task<Json> place(Order order) override;

};
