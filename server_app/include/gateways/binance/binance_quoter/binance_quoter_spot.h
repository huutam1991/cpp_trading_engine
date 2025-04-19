#ifndef BINANCE_QUOTER_SPOT_H
#define BINANCE_QUOTER_SPOT_H

#include <websocket/websocket_client_async.h>
#include <gateways/binance/binance_quoter/binance_quoter.h>

class BinanceQuoterSpot : public BinanceQuoter
{
private:
    std::string m_url = BINANCE_SPOT_URL;
    std::string m_port = BINANCE_SPOT_PORT;
    std::string m_ws_url = BINANCE_SPOT_WS_URL;
    std::string m_ws_port = BINANCE_SPOT_WS_PORT;

    // Websocket to get order data
    std::shared_ptr<WebsocketClientAsync> m_websocket;
    std::string m_listen_key;
    size_t m_schedule_task_id = 0;
    void init_websocket();
    Task<std::string> get_listen_key();
    void add_timer_keep_alive_listen_key(size_t period);
    void del_timer_keep_alive_listen_key();

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    BinanceQuoterSpot(const std::string& key);
    ~BinanceQuoterSpot();

    virtual Json get_trade_result_from_response(Json& response) override;
    virtual Task<Json> get_open_orders(std::string symbol) override;
    virtual TaskVoid cancel_all(std::string symbol) override;
    virtual Task<Json> cancel(Order order) override;
    virtual Task<Json> place(Order order) override;

};

#endif //BINANCE_QUOTER_SPOT_H