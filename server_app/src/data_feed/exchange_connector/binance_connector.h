#ifndef BINANCE_CONNECTOR_H
#define BINANCE_CONNECTOR_H

#include <websocket/websocket_client.h>
#include <mutex>
#include <app_constants.h>

using namespace std;

class BinanceConnector
{
public:
    BinanceConnector(const string binance_ws_url, 
                    const string binance_ws_port, 
                    const string exchange_name);
    ~BinanceConnector();

    void feed_order_book(const string symbol, const int depth = 1);

private:
    size_t get_feed_order_book_id();

private:
    shared_ptr<WebsocketClient> m_websocket;
    WebsocketClientHandle* m_wsh;
    string m_binance_ws_url;
    string m_binance_ws_port;
    string m_exchange_name;

    mutex m_feed_order_book_mutex;

};

#endif