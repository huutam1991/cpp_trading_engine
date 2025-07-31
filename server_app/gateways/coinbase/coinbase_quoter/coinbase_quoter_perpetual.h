#pragma once

#include <websocket/websocket_client_async.h>
#include <gateways/coinbase/coinbase_quoter/coinbase_quoter.h>

class CoinbaseQuoterPerpetual : public CoinbaseQuoter
{
private:
    // Use spot url, need to change later
    std::string m_url = COINBASE_ADVANCE_REALNET_URL;
    std::string m_port = COINBASE_ADVANCE_REALNET_PORT;
    std::string m_ws_url = COINBASE_ADVANCE_REALNET_WS_URL;
    std::string m_ws_port = COINBASE_ADVANCE_REALNET_WS_PORT;

    // For update order result
    std::mutex m_mutex;
    JsonNew m_order_result;
    void update_order_result(const JsonNew& order_result);

    // Websocket to get order data
    std::shared_ptr<WebsocketClientAsync> m_websocket;
    std::string m_listen_key;
    void init_websocket();
    std::string get_listen_key();

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    CoinbaseQuoterPerpetual(const std::string& key);
    ~CoinbaseQuoterPerpetual();

    virtual JsonNew get_trade_result_from_response(JsonNew& response) override;
    virtual Task<JsonNew> get_open_orders(std::string symbol) override;
    virtual TaskVoid cancel_all(std::string symbol) override;
    virtual Task<JsonNew> cancel(Order order) override;
    virtual Task<JsonNew> place(Order order) override;
};
