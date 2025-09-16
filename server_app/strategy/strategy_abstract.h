#pragma once

#include <variant>

#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>
#include <order/order.h>
#include <order_book/order_book_snapshot.h>

struct PriceUpdate
{
    const Instrument* instrument;
    double price;
};

struct TradeUpdate
{
    const Instrument* instrument;
    double price;
    double quantity;
    bool is_buy;
    size_t trade_id;
    size_t timestamp; // In nanoseconds
};

using StrategyUpdateData = std::variant<PriceUpdate, Order, OrderBookSnapShot*, TradeUpdate>;

class StrategyAbstract
{
public:
    EventBase* event_base;

public:
    StrategyAbstract(EventBase* v_event_base) : event_base{v_event_base} {}

    virtual Task<void> init() = 0;
    virtual Task<void> update(StrategyUpdateData data) = 0;

    // For API requests
    virtual std::string get_name() = 0;
    virtual Json get_config() const = 0;
    virtual void update_config(Json& data) = 0;
    virtual Json get_info(Json& params) = 0;
};