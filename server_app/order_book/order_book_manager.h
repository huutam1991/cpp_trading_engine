#include <functional>

#include <utils/util_macros.h>
#include <order_book/order_book_snapshot.h>

class OrderBookManager
{
    Singleton(OrderBookManager);

    std::function<void(OrderBookSnapShot*)> m_order_update_callback = nullptr;

public:
    void register_order_update(std::function<void(OrderBookSnapShot*)>);
    void publish_order_book_snapshot(OrderBookSnapShot* snapshot);
};