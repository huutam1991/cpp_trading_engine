#pragma once

#include <coroutine/epoll_base.h>
#include <network/https_client_websocket/https_client_websocket.h>
#include <network/https_client_request/https_client_request.h>

#include <gateways/binance/binance_quoter/binance_quoter.h>

class BinanceQuoterPerpetual : public BinanceQuoter
{
    std::shared_ptr<HttpsClientRequest> m_client = nullptr;

private:
    std::string m_url = BINANCE_FUTURES_REST_URL;
    std::string m_port = BINANCE_FUTURES_REST_PORT;
    std::string m_ws_url = BINANCE_FUTURES_WS_URL;
    std::string m_ws_port = BINANCE_FUTURES_WS_PORT;

    // EpollBase
    EpollBase* m_epoll_base = nullptr;

    // Websocket to get order data
    std::shared_ptr<HttpsClientWebsocket> m_websocket;
    std::string m_listen_key;
    void init_websocket();
    Task<std::string> get_listen_key();

    // Task to keep listen key alive
    Task<void> m_keep_listen_key_task = nullptr;
    Task<void> keep_listen_key();

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    BinanceQuoterPerpetual(const std::string& key);
    ~BinanceQuoterPerpetual();

    virtual Task<Json> get_open_orders(std::string symbol) override;
    virtual Task<void> cancel_all(std::string symbol) override;
    virtual Task<Json> cancel(Order order) override;
    virtual Task<Json> place(Order order) override;
};
