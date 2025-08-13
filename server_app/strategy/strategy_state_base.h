#pragma once

#include <coroutine/task.h>
#include <enum_reflect/enum_reflect.h>
#include <strategy/strategy_abstract.h>


enum StrategyState
{
    STOP,
    RUN,
    UNKNOWN,
};

struct StrategyStateData
{
    StrategyState state = StrategyState::STOP;

    Json to_json()
    {
        return {
            {"state", enum_reflect::enum_name(state)}
        };
    }

    static StrategyStateData from_json(Json& data)
    {
        StrategyStateData res;

        res.state = data.has_field("state") ?
            enum_reflect::enum_value<StrategyState>((std::string)data["state"]) : StrategyState::STOP;

        return res;
    }
};


class StrategyStateBase
{
public:
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual Task<void> update(StrategyUpdateData data) = 0;
    virtual Json get_info() = 0;
};