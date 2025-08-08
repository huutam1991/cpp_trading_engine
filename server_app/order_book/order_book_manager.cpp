#include <order_book/order_book_manager.h>

void OrderBookManager::register_order_update(std::function<void(OrderBookSnapShot*)> callback)
{
    m_order_update_callback = std::move(callback);
}

void OrderBookManager::publish_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    if (m_order_update_callback)
    {
        m_order_update_callback(snapshot);
    }
}