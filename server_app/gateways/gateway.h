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
    ExchangeId m_exchange_id;
    std::function<void(const Instrument*,double)> m_price_update_callback;
    EventBase* m_event_base = nullptr;

    Gateway();
    virtual std::vector<Instrument> fetch_instruments();
    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) = 0;
    virtual Task<void> cancel_all_on_exchange(std::string symbol) = 0;
    virtual Task<Json> cancel_on_exchange(Order order) = 0;
    virtual Task<Json> place_on_exchange(Order order) = 0;

public:
    virtual ExchangeId get_exchange() = 0;
    std::string get_name();
    void register_price_update(std::function<void(const Instrument*,double)> price_update_callback);
    void check_remove_canceled_orders(std::string symbol);
    void cancel_all(std::string symbol);
    void place(Order order);
    void cancel(Order order);

    void init();

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) = 0;

    // Util methods
    virtual Task<Json> get_balances() = 0;
};
