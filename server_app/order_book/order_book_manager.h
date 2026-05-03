#include <functional>

#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>
#include <utils/util_macros.h>
#include <order_book/order_book_snapshot.h>

class OrderBookManager
{
    Singleton(OrderBookManager);

    EventBase* m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER_BOOK);
    std::vector<std::function<void(OrderBookSnapShotObject)>> m_update_callbacks;

    Task<void> run_update_order_book_snapshot(OrderBookSnapShotObject snapshot);

public:
    void register_update(std::function<void(OrderBookSnapShotObject)> callback);
    void publish_order_book_snapshot(OrderBookSnapShotObject snapshot);
};