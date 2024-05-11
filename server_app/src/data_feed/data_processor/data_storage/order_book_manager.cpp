#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <json/json.h>
#include <utils.h>

bool OrderBookManager::add_order_book(string symbol, const int capacity)
{
    if (m_order_book_map.find(symbol) == m_order_book_map.end())
    {
        shared_ptr<OrderBook> order_book = make_shared<OrderBook>(capacity);
        // m_order_book_map.insert(make_pair(symbol, order_book));
        m_order_book_map[symbol] = order_book;

        ADD_LOG("Add order book " << symbol << " count = " << m_order_book_map.size());

        return true;
    }

    return false;
}

bool OrderBookManager::remove_order_book(string symbol, const bool force_remove)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr) 
    {
        if (force_remove)
        {
            // remove from map -> will delete from memory
            m_order_book_map.erase(symbol);
            return true;
        }
        else 
        {
            if (order_book->get_total_observers() == 0)
            {
                // remove from map -> will delete from memory
                m_order_book_map.erase(symbol);
                return true;
            }
        }
    } 

    return false;
}

shared_ptr<OrderBook> OrderBookManager::get_order_book_by_symbol(string symbol)
{
    if (symbol.length() < 1) return nullptr;

    if (m_order_book_map.find(symbol) != m_order_book_map.end())
    {
        return m_order_book_map[symbol];
    }
    else {
        return nullptr;
    }
}

void OrderBookManager::add_new_book_ticker(string symbol, Json& json)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr) 
    {
        order_book->add_order_book(json);
        // send to all subscribers
        order_book->notify_observers();
    } 
}

void OrderBookManager::update_depth(string symbol, Json& json)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr) 
    {
        order_book->update_latest_depth(json);
    } 
}

void OrderBookManager::update_agg_trade(string symbol, Json& json)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr) 
    {
        order_book->update_latest_price(json);
        // send to all subscribers
        order_book->notify_observers();
    } 
}

void OrderBookManager::update_kline(string symbol, Json& json)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr) 
    {
        Json book_ticker;
        book_ticker["p"] = (long double)json["c"];
        book_ticker["b"] = (long double)json["c"];
        book_ticker["a"] = (long double)json["c"];
        book_ticker["B"] = 0.0;
        book_ticker["A"] = 0.0;

        Json a = Json::create_array();
        a.push_back(book_ticker["a"]);
        a.push_back(book_ticker["A"]);
        Json A = Json::create_array();
        A.push_back(a);
        book_ticker["asks"] = A;

        Json b = Json::create_array();
        b.push_back(book_ticker["b"]);
        b.push_back(book_ticker["B"]);
        Json B = Json::create_array();
        B.push_back(b);
        book_ticker["bids"] = B;

        order_book->add_order_book(book_ticker);
        // send to all subscribers
        order_book->notify_observers();
    } 
}

void OrderBookManager::update_order_book(Json& json)
{
    // ADD_LOG("Binance order book: " << json.get_string_value());
    // thread safe mode
    unique_lock lock(m_update_order_book_mutex);

    string symbol = "";
    if (json.has_field("s"))
        symbol = (string&&)json["s"];

    if ((string&&)json["e"] == "bookTicker")
    {
        // ADD_LOG("Update bookTicker " << symbol);
        add_new_book_ticker(symbol, json);
    }
    else if ((string&&)json["e"] == "depthUpdate")
    {
        // ADD_LOG("Update depthUpdate " << symbol);
        update_depth(symbol, json);
    }    
    else if ((string&&)json["e"] == "aggTrade")
    {
        // ADD_LOG("Update aggTrade " << json.get_string_value());
        update_agg_trade(symbol, json);
    }    
    else if ((string&&)json["e"] == "kline")
    {
        // ADD_LOG("Update kline " << json.get_string_value());
        update_kline(symbol, json);
    }    
}

void OrderBookManager::subscribe_symbol(const string& symbol, observer::Observer* observer)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr)
    {
        order_book->register_observer(observer);
    }
}

void OrderBookManager::unsubscribe_symbol(const string& symbol, observer::Observer* observer)
{
    shared_ptr<OrderBook> order_book = get_order_book_by_symbol(symbol);
    if (order_book != nullptr)
    {
        order_book->remove_observer(observer);
    }
}
