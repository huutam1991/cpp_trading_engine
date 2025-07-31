#pragma once

#include <websocket/websocket_client_async.h>
#include <coroutine/event_base_manager.h>
#include <gateways/coinbase/coinbase_quoter/coinbase_quoter.h>

class CoinbaseQuoterSpot : public CoinbaseQuoter
{
private:
    std::string m_url = COINBASE_ADVANCE_REALNET_URL;
    std::string m_port = COINBASE_ADVANCE_REALNET_PORT;
    std::string m_ws_url = COINBASE_ADVANCE_REALNET_WS_URL;
    std::string m_ws_port = COINBASE_ADVANCE_REALNET_WS_PORT;

    EventBase* m_event_base = nullptr;

    // Websocket to get order data
    std::shared_ptr<WebsocketClientAsync> m_websocket;
    std::string m_listen_key;
    void init_websocket();
    Task<std::string> get_listen_key();
    TaskVoid keep_listen_key();

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    CoinbaseQuoterSpot(const std::string& key);
    ~CoinbaseQuoterSpot();

    virtual JsonNew get_trade_result_from_response(JsonNew& response) override;
    virtual Task<JsonNew> get_open_orders(std::string symbol) override;
    virtual TaskVoid cancel_all(std::string symbol) override;
    virtual Task<JsonNew> cancel(Order order) override;
    virtual Task<JsonNew> place(Order order) override;

};
