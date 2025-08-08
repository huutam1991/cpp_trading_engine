#include <order_book/order_book_manager.h>

void OrderBookManager::register_update(std::function<void(OrderBookSnapShot*)> callback)
{
    m_update_callback = std::move(callback);
}

void OrderBookManager::publish_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    auto task = run_update_order_book_snapshot(snapshot);

    if (m_event_base == nullptr)
    {
        m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY);
    }
    task.start_running_on(m_event_base);
}

Task<void> OrderBookManager::run_update_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    if (m_update_callback)
    {
        m_update_callback(snapshot);
    }

    co_return;
}