#pragma once

#include <map>

#include <coroutine/task_void.h>
#include <json/json.h>
#include <gateways/binance/binance_market_data/order_book/order_book_websocket/order_book_websocket.h>
#include <gateways/binance/binance_market_data/order_book/order_book_rest/order_book_rest.h>

class OrderBook
{
public:
    OrderBook(const std::string& symbol, size_t depth_level, net::io_context& ioc, EventBase* event_base);
    TaskVoid send_request_get_full_order_book();

    bool is_not_synced();
    void print_order_book();

private:
    std::string m_symbol;
    size_t m_depth_level;
    OrderBookWebsocket m_order_book_websocket;
    OrderBookRest m_order_book_rest;

    // Bid, Ask
    std::map<double, double, std::greater<double>> m_bids;
    std::map<double, double, std::less<double>> m_asks;

    // For logic to apply orderbook's updates
    bool m_snapshot_loaded = false;
    bool m_ws_waiting_first_event = true;
    size_t m_snapshot_last_update_id = 0;
    size_t m_ws_last_update_id = 0;

    void OnOrderbookWs(std::string data);
    void OnOrderbookRest(std::string data);

    void apply_snapshot(Json& snapshsot);
};