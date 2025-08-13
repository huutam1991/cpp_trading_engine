#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <order/order_manager.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyTrendFollowStateStop : public StrategyStateBase
{
    struct OrderGap
    {
        double bid;
        double ask;
        double gap;
        double bid_quantity;
        double ask_quantity;
        size_t time;

        Json to_json() const
        {
            Json json;
            json["bid"] = bid;
            json["ask"] = ask;
            json["gap"] = gap;
            json["bid_quantity"] = bid_quantity;
            json["ask_quantity"] = ask_quantity;
            json["time"] = time;

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
            gap.time = data["time"];

            return gap;
        }
    };

    std::string m_db_name;
    std::unordered_map<size_t, SavableObject<OrderGap>> m_order_gap_list;

public:
    StrategyTrendFollowStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    // virtual Json get_open_orders() override;
};
