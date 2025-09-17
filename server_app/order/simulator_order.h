#pragma once

// This is a simulator for order placement and cancellation. It does not interact with any real exchange.

#include <data_model/savable_object.h>
#include <order/order.h>

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

public:
    static void init();
};