#include <functional>

#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>
#include <utils/util_macros.h>
#include <order_book/order_book_snapshot.h>

class OrderBookManager
{
    Singleton(OrderBookManager);

    EventBase* m_event_base = nullptr;
    std::function<void(OrderBookSnapShot*)> m_update_callback = nullptr;

    Task<void> run_update_order_book_snapshot(OrderBookSnapShot* snapshot);

public:
    void register_update(std::function<void(OrderBookSnapShot*)>);
    void publish_order_book_snapshot(OrderBookSnapShot* snapshot);
};