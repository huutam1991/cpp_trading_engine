#ifndef GATEWAY_H
#define GATEWAY_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <order/order.h>

class Gateway
{
protected:
    std::function<void(double)> m_price_update_callback;

public:
    void register_price_update(std::function<void(double)> price_update_callback);

    virtual Json place(Order order);

};

#endif //GATEWAY_H