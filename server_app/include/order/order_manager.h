#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <thread>
#include <functional>
#include <unordered_map>

#include <util_macros.h>
#include <data_model/data_model.h>

#include <app_utils.h>
#include <order/order.h>

class OrderManager
{
    Singleton(OrderManager);

private:
    std::unordered_map<OrderId, DataModel> m_order_list;


public:
    static OrderId generate_order_id();

    void init();
    void create_order_data_model(OrderId order_id);
    DataModel find_order_by_id(OrderId order_id);
    void update_order(Order order);

};

#endif // ORDER_MANAGER_H