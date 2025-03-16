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

    void init();

public:
    static OrderId generate_order_id();

};

#endif // ORDER_MANAGER_H