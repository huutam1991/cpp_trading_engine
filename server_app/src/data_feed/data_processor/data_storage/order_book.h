#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <ring_buffer/ring_buffer.h>
#include <mutex>
#include <condition_variable>
#include <bits/stdc++.h>
#include <json/json.h>
#include <observer.h>

using namespace std;

class OrderBook : observer::Subject
{
public:
    void register_observer(observer::Observer *observer) override;
    void remove_observer(observer::Observer *observer) override;
    void notify_observers() override;

    int get_total_observers();

public:
    OrderBook(const int capacity);
    ~OrderBook();

    void add_order_book(Json& order_book);
    void update_latest_depth(Json& order_book_depth);
    void update_latest_price(Json& agg_trade);

    RingBuffer<Json>* read_ring_buffer();
    void begin_read_ring_buffer();
    void end_read_ring_buffer();
    
    int get_ring_buffer_size();

private:
    void begin_write_ring_buffer();
    void end_write_ring_buffer();

private:
    RingBuffer<Json> *m_ring_buffer = nullptr;
    string m_symbol;
    
    // thread safe: readers - writer lock
    mutex m_mutex;
    condition_variable m_cond_var;
    int m_num_readers_active;
    int m_num_writers_waiting;
    bool m_writer_active;
    
};

#endif