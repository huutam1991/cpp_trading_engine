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

    Json to_json() const
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
    Task<void> update(StrategyUpdateData data)
    {
        if (std::holds_alternative<PriceUpdate>(data))
        {
            handle_price_update(std::get<PriceUpdate>(data));
        }
        else if (std::holds_alternative<TradeUpdate>(data))
        {
            handle_trade_update(std::get<TradeUpdate>(data));
        }
        else if (std::holds_alternative<OrderBookSnapShotObject>(data))
        {
            OrderBookSnapShotObject snapshot = std::get<OrderBookSnapShotObject>(data);
            handle_order_book_snapshot(snapshot.object);
        }
        else
        {
            handle_order_update(std::get<Order>(data));
        }

        co_return;
    }

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual Json get_info() = 0;

protected:
    virtual void handle_price_update(PriceUpdate& price_update) = 0;
    virtual void handle_trade_update(TradeUpdate& trade_update) = 0;
    virtual void handle_order_book_snapshot(OrderBookSnapShot* snapshot) = 0;
    virtual void handle_order_update(Order& order) = 0;
};