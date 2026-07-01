#pragma once

#include <unordered_set>
#include <spdlog/spdlog.h>

#include <utils/util_macros.h>
#include <json/json.h>
#include <coroutine/task.h>
#include <data_model/savable_object.h>
#include <network/external_request/https_client_async.h>

#include <instrument/instrument.h>
#include <order/order_manager.h>
#include <app_constants.h>

class OrderEntry
{
protected:
    ExchangeId m_exchange_id;
    EventBase* m_event_base = nullptr;

    OrderEntry(ExchangeId exchange_id, EventBase* event_base) : m_exchange_id(exchange_id), m_event_base(event_base) {};

    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) = 0;
    virtual Task<void> cancel_all_on_exchange(std::string symbol) = 0;
    virtual Task<Json> cancel_on_exchange(Order order) = 0;
    virtual Task<Json> place_on_exchange(Order order) = 0;

public:
    void check_remove_canceled_orders(std::string symbol);
    void cancel_all(std::string symbol);
    void place(Order order);
    void cancel(Order order);

    // Util methods
    virtual Task<Json> get_balances() = 0;
    virtual Task<Json> get_positions() = 0;
};
