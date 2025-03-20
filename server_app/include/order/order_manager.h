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

class OrderManager
{
    Singleton(OrderManager);

private:
    std::unordered_map<OrderId, Order> m_order_list;
    std::function<void(Order&)> m_order_update_callback = nullptr;

    // For handle order create / update
    OrderDataModelHelper m_order_data_model_helper;
    TaskVoid handle_update_order(Order order);
    Order find_order_by_id(OrderId order_id);

public:
    static OrderId generate_order_id();

    void init();
    void register_order_update(std::function<void(Order&)>);
    void update_order(Order order);

    // For getting order data
    // Return Future because the order's data might not be arrive yet
    Future<Order> get_order_data(OrderId order_id);

};

#endif // ORDER_MANAGER_H