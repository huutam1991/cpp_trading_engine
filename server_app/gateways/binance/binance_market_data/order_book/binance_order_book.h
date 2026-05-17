#pragma once

#include <map>

#include <coroutine/task.h>
#include <coroutine/task.h>
#include <json/json.h>

#include <order_book/order_book_snapshot.h>
#include <order_book/order_book_manager.h>
#include <gateways/binance/binance_market_data/order_book/order_book_websocket/order_book_websocket.h>
#include <gateways/binance/binance_market_data/order_book/order_book_rest/order_book_rest.h>

class BinanceOrderBook
{
public:
    BinanceOrderBook(const std::string& symbol, size_t depth_level, EpollBase* event_base);
    Task<void> send_request_get_full_order_book();

    bool is_not_synced();
    void print_order_book();

private:
    std::string m_symbol;
    const Instrument* m_instrument = nullptr;
    size_t m_depth_level;
    EpollBase* m_event_base;
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

    Task<void> release_current_update(Json update);

    void OnOrderbookWs(std::string data);
    void OnOrderbookRest(std::string data);

    void apply_snapshot(Json& snapshsot);
    void export_snapshot();
};