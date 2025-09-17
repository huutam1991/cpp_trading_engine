#pragma once

// This is a simulator for order placement and cancellation. It does not interact with any real exchange.

#include <unordered_map>

#include <data_model/savable_object.h>
#include <coroutine/event_base_manager.h>
#include <coroutine/task.h>
#include <order/order_manager.h>
#include <strategy/strategy_abstract.h>

class SimulatorOrder
{

public:
    enum SimulatorState
    {
        NONE,
        ACTIVE,
    };

    struct SimulatorConfig
    {
        SimulatorState state;

        SimulatorConfig static from_json(Json& data)
        {
            SimulatorConfig config;
            config.state = data.has_field("state") ? enum_reflect::enum_value<SimulatorState>((std::string)data["state"]) : SimulatorState::NONE;
            return config;
        }

        Json to_json() const
        {
            return {
                {"state", enum_reflect::enum_name(state)}
            };
        }
    };

    static SavableObject<SimulatorConfig>& get_config();
    static EventBase* get_event_base();
    static std::unordered_map<const Instrument*, std::unordered_map<OrderId, Order>>& get_order_list();

public:
    static void init();
    static bool is_active();

    static void place(Order order);
    static void cancel(Order order);
    static void cancel_all(std::string symbol);
    static void price_update(PriceUpdate data);
    static Task<void> execute_place(Order order);
    static Task<void> execute_cancel(Order order);
    static Task<void> execute_cancel_all(std::string symbol);
    static Task<void> execute_price_update(PriceUpdate data);
};