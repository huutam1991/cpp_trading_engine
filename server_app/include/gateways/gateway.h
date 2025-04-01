#ifndef GATEWAY_H
#define GATEWAY_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <order/order_manager.h>
#include <coroutine/task.h>

class Gateway
{
protected:
    std::function<void(double)> m_price_update_callback;
    EventBase* m_event_base = nullptr;

    Gateway();
    virtual TaskVoid cancel_all_on_exchange() = 0;
    virtual Task<Json> place_on_exchange(Order order) = 0;

public:
    void register_price_update(std::function<void(double)> price_update_callback);
    void cancel_all();
    void place_none_wait(Order order);
    Task<Order> place(Order order, Order::Status wait_status = Order::Status::FILLED);

    virtual void subscribe_symbol(const std::string& symbol) = 0;

    // Util methods
    virtual Task<Json> get_balances() = 0;
    virtual double round_up_quantity(const std::string& type, const std::string& symbol, double quantity) = 0;
};

#endif //GATEWAY_H