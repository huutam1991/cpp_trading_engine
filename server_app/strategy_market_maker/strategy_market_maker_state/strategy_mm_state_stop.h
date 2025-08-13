#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyMarketMakerStateStop : public StrategyStateBase
{
    struct OrderGap
    {
        double bid;
        double ask;
        double gap;
        double bid_quantity;
        double ask_quantity;

        Json to_json() const
        {
            Json json;
            json["bid"] = bid;
            json["ask"] = ask;
            json["gap"] = gap;
            json["bid_quantity"] = bid_quantity;
            json["ask_quantity"] = ask_quantity;

            return json;
        }

        static OrderGap from_json(Json& data)
        {
            OrderGap gap;
            gap.bid = data["bid"];
            gap.ask = data["ask"];
            gap.gap = data["gap"];
            gap.bid_quantity = data["bid_quantity"];
            gap.ask_quantity = data["ask_quantity"];

            return gap;
        }
    };

    std::unordered_map<OrderId, SavableObject<OrderGap>> m_order_list;

public:
    StrategyMarketMakerStateStop();

    Json get_info();

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;

    // virtual Json get_open_orders() override;
};
