#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <mutex>
#include <functional>
#include <unordered_map>
#include <queue>

#include <util_macros.h>
#include <coroutine/task_void.h>
#include <coroutine/future.h>

#include <order/order.h>
#include <order/order_data_model_helper.h>

namespace std {
    template<>
    struct hash<std::pair<size_t, Order::Status>> {
        std::size_t operator()(const std::pair<OrderId, Order::Status>& p) const noexcept {
            return std::hash<size_t>{}(p.first) ^ (std::hash<int>{}(static_cast<int>(p.second)) << 1);
        }
    };
}

class OrderManager
{
    Singleton(OrderManager);

private:
    std::unordered_map<OrderId, Order> m_order_list;
    std::function<void(Order&)> m_order_update_callback = nullptr;
    EventBase* m_order_event_base = nullptr;

    // For handling order create / update
    OrderDataModelHelper m_order_data_model_helper;
    TaskVoid handle_update_order(Order order);

    // For getting order with a specific status
    std::unordered_map<std::pair<OrderId, Order::Status>, std::vector<Future<Order>::FutureValue>> m_order_future_value;
    TaskVoid handle_add_order_future_value(Future<Order>::FutureValue value, OrderId order_id, Order::Status status);
    void check_set_future_value_for_order(OrderId order_id, Order::Status status);
    void add_order_future_value(Future<Order>::FutureValue value, OrderId order_id, Order::Status status);

public:
    // Helper method
    static OrderId generate_order_id();
    Order& get_order_by_id(OrderId order_id);
    bool is_valid_order(OrderId order_id)
    {
        return m_order_list.find(order_id) != m_order_list.end();
    }
    Order::Status get_order_status(OrderId order_id)
    {
        return m_order_list.find(order_id) != m_order_list.end() ?
            m_order_list[order_id].status :
            Order::Status::NOT_AVAILABLE;
    }
    std::vector<OrderId> get_open_orders();

    void init();
    void register_order_update(std::function<void(Order&)>);
    void set_cancel_order(OrderId order_id);
    void update_order(Order order);

    // Wait order until it get a specific status
    Future<Order> wait_for_order_status(OrderId order_id, Order::Status status);

};

#endif // ORDER_MANAGER_H