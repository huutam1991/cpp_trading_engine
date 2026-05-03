#include <order_book/order_book_manager.h>

void OrderBookManager::register_update(std::function<void(OrderBookSnapShotObject)> callback)
{
    m_update_callbacks.push_back(std::move(callback));
}

void OrderBookManager::publish_order_book_snapshot(OrderBookSnapShotObject snapshot)
{
    auto task = run_update_order_book_snapshot(snapshot);
    task.start_running_on(m_event_base);
}

Task<void> OrderBookManager::run_update_order_book_snapshot(OrderBookSnapShotObject snapshot)
{
    for (auto& callback : m_update_callbacks)
    {
        callback(snapshot);
    }

    co_return;
}