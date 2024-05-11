#include <data_feed/data_processor/data_storage/order_book.h>
#include <utils.h>

void OrderBook::register_observer(observer::Observer *observer) 
{
    m_observers.push_back(observer);
}

void OrderBook::remove_observer(observer::Observer *observer) 
{
    // find the observer
    auto iterator = std::find(m_observers.begin(), m_observers.end(), observer);

    if (iterator != m_observers.end()) { // observer found
        m_observers.erase(iterator); // remove the observer
    }
}

void OrderBook::notify_observers() 
{
    for (observer::Observer *observer : m_observers) { // notify all observers
        observer->on_notify(this);
    }
}

int OrderBook::get_total_observers()
{
    return m_observers.size();
}

OrderBook::OrderBook(const int capacity)
{
    m_ring_buffer = new RingBuffer<Json>(capacity);
    m_num_readers_active = 0;
    m_num_writers_waiting = 0;
    m_writer_active = false;
}

OrderBook::~OrderBook()
{
    begin_write_ring_buffer();

    if (m_ring_buffer)
    {
        delete m_ring_buffer;
    }

    end_write_ring_buffer();
}

void OrderBook::begin_read_ring_buffer()
{
    unique_lock<mutex> lock(m_mutex);
    while (m_writer_active || m_num_writers_waiting >  0)
        m_cond_var.wait(lock);

    m_num_readers_active++;
}

void OrderBook::end_read_ring_buffer()
{
    unique_lock<mutex> lock(m_mutex);
    m_num_readers_active--;
    if (m_num_readers_active == 0)
        m_cond_var.notify_all();
}

void OrderBook::begin_write_ring_buffer()
{
    unique_lock<mutex> lock(m_mutex);
    m_num_writers_waiting++;
    while (m_writer_active || m_num_readers_active >  0)
        m_cond_var.wait(lock);

    m_num_writers_waiting--;
    m_writer_active = true; 
}

void OrderBook::end_write_ring_buffer()
{
    unique_lock<mutex> lock(m_mutex);
    m_writer_active = false; 
    m_cond_var.notify_all();
}

void OrderBook::add_order_book(Json& order_book)
{
    begin_write_ring_buffer();

    // copy depth from older order book
    if (m_ring_buffer->size() > 0 && m_ring_buffer->first()["asks"].size() > 0) 
    {
        // Json* lastest = &(m_ring_buffer->first());
        Json asks = m_ring_buffer->first()["asks"];
        Json bids = m_ring_buffer->first()["bids"];
        asks[0] = order_book["asks"][0];
        bids[0] = order_book["bids"][0];
        order_book["asks"] = asks;
        order_book["bids"] = bids;        
        if (!order_book.has_field("p"))
        {
            order_book["p"] = m_ring_buffer->first()["p"];        
            order_book["q"] = m_ring_buffer->first()["q"];        
        }
    }
    else if (m_ring_buffer->size() == 0) 
    {
        order_book["p"] = ((long double)order_book["b"] + (long double)order_book["a"]) / 2;        
        order_book["q"] = 0;        
    }
    m_ring_buffer->unshift(order_book.clone());

    // ADD_LOG("add_order_book: " << m_ring_buffer->first().get_string_value());

    end_write_ring_buffer();
}

void OrderBook::update_latest_depth(Json& order_book_depth)
{
    if (m_ring_buffer->is_empty())
        return;
        
    begin_write_ring_buffer();

    if (m_ring_buffer->size() > 0)
    {
        m_ring_buffer->first()["asks"] = order_book_depth["asks"];
        m_ring_buffer->first()["bids"] = order_book_depth["bids"];
    }

    end_write_ring_buffer();
}

void OrderBook::update_latest_price(Json& agg_trade)
{
    if (m_ring_buffer->is_empty())
        return;
        
    begin_write_ring_buffer();

    if (m_ring_buffer->size() > 0)
    {
        m_ring_buffer->first()["p"] = agg_trade["p"];
        m_ring_buffer->first()["q"] = agg_trade["q"];
    }

    // ADD_LOG("update_latest_price: " << m_ring_buffer->first().get_string_value());

    end_write_ring_buffer();
}

RingBuffer<Json>* OrderBook::read_ring_buffer()
{
    return m_ring_buffer;
}

int OrderBook::get_ring_buffer_size()
{
    if (m_ring_buffer == nullptr) return 0;

    return m_ring_buffer->size();
}
