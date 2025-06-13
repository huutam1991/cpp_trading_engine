#pragma once

#include <unordered_set>
#include <spdlog/spdlog.h>

#include <utils/util_macros.h>
#include <json/json.h>
#include <coroutine/task.h>
#include <data_model/savable_object.h>
#include <external_request/https_client_async.h>
#include <ioc_pool.h>

#include <instrument/instrument.h>
#include <order/order_manager.h>
#include <app_constants.h>

class Gateway
{
protected:
    std::string m_gateway_name;
    std::unordered_map<std::string, SavableObject<Instrument>> m_instruments;
    std::function<void(std::string,double)> m_price_update_callback;
    EventBase* m_event_base = nullptr;

    Gateway();
    virtual std::string get_name() = 0;
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

    Instrument* get_instrument_by_symbol(const std::string& symbol);

    virtual void init();
    virtual void subscribe_symbol(std::vector<std::string> symbols) = 0;

    // Util methods
    virtual Task<Json> get_balances() = 0;
    virtual double round_up_quantity(const std::string& type, const std::string& symbol, double quantity) = 0;
    virtual size_t get_lot_size(const std::string& type, const std::string& symbol) = 0;
};
