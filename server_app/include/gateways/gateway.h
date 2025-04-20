#ifndef GATEWAY_H
#define GATEWAY_H

#include <unordered_set>

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <order/order_manager.h>
#include <coroutine/task.h>

class Gateway
{
protected:
    std::function<void(std::string,double)> m_price_update_callback;
    EventBase* m_event_base = nullptr;

    Gateway();
    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) = 0;
    virtual TaskVoid cancel_all_on_exchange(std::string symbol) = 0;
    virtual Task<Json> cancel_on_exchange(Order order) = 0;
    virtual Task<Json> place_on_exchange(Order order) = 0;

public:
    void register_price_update(std::function<void(std::string,double)> price_update_callback);
    void check_remove_canceled_orders(std::string symbol);
    void cancel_all(std::string symbol);
    void place_none_wait(Order order);
    void cancel(Order order);
    Task<Order> place(Order order, Order::Status wait_status = Order::Status::FILLED);

    virtual void subscribe_symbol(std::vector<std::string> symbols) = 0;

    // Util methods
    virtual Task<Json> get_balances() = 0;
    virtual double round_up_quantity(const std::string& type, const std::string& symbol, double quantity) = 0;
};

#endif //GATEWAY_H