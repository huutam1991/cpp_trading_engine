#pragma once

#include <map>

#include <coroutine/task.h>
#include <json/json.h>

class Orderbook
{
public:
    Orderbook(const std::string& symbol, EventBase* event_base);
    Task<void> send_request_get_full_order_book();

    bool is_not_synced();
    void print_order_book();

private:
    std::string m_symbol;
    EventBase* m_event_base;
    size_t m_depth_level;

    // Bid, Ask
    std::map<double, double, std::greater<double>> m_bids;
    std::map<double, double, std::less<double>> m_asks;

    Task<void> apply_snapshot(Json& snapshot);
    Task<void> apply_book_levels(Json& levels);
};