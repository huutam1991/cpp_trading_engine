#ifndef GATEWAY_H
#define GATEWAY_H

#include <util_macros.h>
#include <order/order.h>

class Gateway
{
public:
    virtual void place(Order order) = 0;

};

#endif //GATEWAY_H