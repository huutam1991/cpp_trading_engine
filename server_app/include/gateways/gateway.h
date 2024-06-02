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

    virtual void subscribe_symbol(const std::string& symbol) = 0;
    virtual Json place(Order order) = 0;

    // Util methods
    virtual Json get_balances() = 0;
    virtual double round_up_quantity(const std::string& symbol, double quantity) = 0;
};

#endif //GATEWAY_H