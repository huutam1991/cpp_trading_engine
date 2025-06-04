#pragma once

#include <coroutine/task_void.h>
#include <strategy/strategy_abstract.h>

enum StrategyState
{
    STOP,
    RUN,
};

struct StrategyStateData
{
    StrategyState state = StrategyState::STOP;

    static inline std::string to_string(StrategyState data)
    {
        return data == StrategyState::RUN ? "RUN" : "STOP";
    }

    static inline StrategyState from_string(const std::string& data)
    {
        return data == "RUN" ? StrategyState::RUN : StrategyState::STOP;
    }

    Json to_json()
    {
        return {
            {"state", to_string(state)}
        };
    }

    static StrategyStateData from_json(Json& data)
    {
        StrategyStateData res;

        res.state = data.has_field("state") ? 
            from_string((std::string)data["state"]) : StrategyState::STOP;

        return res;
    }
};


class StrategyStateBase
{
public:
    virtual void begin();
    virtual void end();
    virtual TaskVoid update(StrategyUpdateData data);
};