#ifndef ORDER_BOOK_MANAGER_H
#define ORDER_BOOK_MANAGER_H

#include <bits/stdc++.h>

#include <util_macros.h>
#include <unordered_map>
#include <data_feed/data_processor/data_storage/order_book.h>
#include <observer.h>
#include <ring_buffer/ring_buffer.h>

class Json;
using namespace std;

class OrderBookManager
{
    Singleton(OrderBookManager)

public:
    bool add_order_book(string symbol, const int capacity = DAFAULT_RING_BUFFER_SIZE);
    bool remove_order_book(string symbol, const bool force_remove = false);

    shared_ptr<OrderBook> get_order_book_by_symbol(string symbol);

    void update_order_book(Json& json);

    void subscribe_symbol(const string& symbol, observer::Observer* observer);
    void unsubscribe_symbol(const string& symbol, observer::Observer* observer);

protected:
    void add_new_book_ticker(string symbol, Json& json);
    void update_depth(string symbol, Json& json);
    void update_agg_trade(string symbol, Json& json);
    void update_kline(string symbol, Json& json);

protected:
    unordered_map<string, shared_ptr<OrderBook>> m_order_book_map;

private:
    mutex m_update_order_book_mutex;

};

#endif