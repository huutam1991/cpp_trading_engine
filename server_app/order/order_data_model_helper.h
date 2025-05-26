#pragma once

#include <unordered_map>

#include <utils/util_macros.h>
#include <data_model/data_model.h>
#include <coroutine/task_void.h>

#include <app_utils.h>
#include <order/order.h>

class OrderDataModelHelper
{
private:
    std::unordered_map<OrderId, DataModel> m_order_list;

    // For handle order create / update
    void create_order_data_model(OrderId order_id);
    DataModel get_order_by_id(OrderId order_id);

public:
    std::unordered_map<OrderId, Order> load_order();

    // For updating order
    void update_order(Order& order);
    TaskVoid task_update_order(Order order);
};