#pragma once

#include <websocket/websocket_client_async.h>
#include <gateways/binance/binance_quoter/binance_quoter.h>

class BinanceQuoterPerpetual : public BinanceQuoter
{
private:
    std::string m_url = BINANCE_FUTURES_URL;
    std::string m_port = BINANCE_FUTURES_PORT;
    std::string m_ws_url = BINANCE_FUTURES_WS_URL;
    std::string m_ws_port = BINANCE_FUTURES_WS_PORT;

    // For update order result
    std::mutex m_mutex;
    Json m_order_result;
    void update_order_result(const Json& order_result);

    // Websocket to get order data
    std::shared_ptr<WebsocketClientAsync> m_websocket;
    std::string m_listen_key;
    void init_websocket();
    std::string get_listen_key();
    void add_timer_keep_alive_listen_key(size_t period);

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    BinanceQuoterPerpetual(const std::string& key);
    ~BinanceQuoterPerpetual();

    virtual Json get_trade_result_from_response(Json& response) override;
    virtual Task<Json> get_open_orders(std::string symbol) override;
    virtual TaskVoid cancel_all(std::string symbol) override;
    virtual Task<Json> cancel(Order order) override;
    virtual Task<Json> place(Order order) override;
};
