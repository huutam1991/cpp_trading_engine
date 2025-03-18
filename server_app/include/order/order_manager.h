#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <mutex>
#include <functional>
#include <unordered_map>
#include <queue>

#include <util_macros.h>
#include <data_model/data_model.h>
#include <coroutine/task_void.h>
#include <coroutine/future.h>

#include <app_utils.h>
#include <order/order.h>

class OrderManager
{
    Singleton(OrderManager);

private:
    std::unordered_map<OrderId, DataModel> m_order_list;
    std::queue<Order> m_order_update_queue;
    std::function<void(Order&)> m_order_update_callback = nullptr;
    std::mutex m_order_manager_mutex;

    // For handle order create / update
    void create_order_data_model(OrderId order_id);
    DataModel find_order_by_id(OrderId order_id);
    void handle_update_order(Order order);

    // For coroutine task
    Future<bool>::FutureValue m_has_order_update;
    TaskVoid check_update_order();
    Future<bool> wait_new_order_update();

public:
    static OrderId generate_order_id();

    void init();
    void register_order_update(std::function<void(Order&)>);
    void update_order(Order order);

    // For getting order data
    // Return Future because the order's data might not be arrive yet
    Future<Order> get_order_result(OrderId order_id);

};

#endif // ORDER_MANAGER_H