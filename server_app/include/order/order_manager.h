#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <thread>
#include <functional>

#include <order/order.h>
#include <util_macros.h>

class OrderManager
{
    Singleton(OrderManager);

private:

public:
    uint64_t generate_order_id();

};

#endif // ORDER_MANAGER_H