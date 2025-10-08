#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_market_maker/strategy_market_maker_config.h>
#include <volume_stat/volume_stat.h>
#include <pnl/pnl.h>

class StrategyMarketMakerStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMarketMakerConfig& m_config;
    const Instrument* m_instrument = nullptr;
    EventBase* m_event_base = nullptr;
    VolumeStat& m_volume_stat;
    PnL& m_pnl;
    VolumeStat m_15_mins_volume_stat;
    size_t m_start_time;

public:
    StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config, VolumeStat& volume_stat, PnL& pnl);

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    void on_config_change();

private:
    struct FillStat
    {
        size_t filled_buy_order_count = 0;
        size_t filled_sell_order_count = 0;
        size_t filled_at_volume_lower_0_5 = 0;
        size_t filled_at_volume_lower_1 = 0;
        size_t filled_at_volume_lower_5 = 0;
        size_t filled_at_volume_lower_10 = 0;
        size_t filled_at_volume_higher_10 = 0;

        void update(double filled_volume)
        {
            if (filled_volume == 0.4)
            {
                filled_at_volume_lower_0_5++;
            }
            else if (filled_volume == 0.3)
            {
                filled_at_volume_lower_1++;
            }
            else if (filled_volume == 0.2)
            {
                filled_at_volume_lower_5++;
            }
            else if (filled_volume == 0.15)
            {
                filled_at_volume_lower_10++;
            }
            else
            {
                filled_at_volume_higher_10++;
            }
        }

        double total()
        {
            return filled_at_volume_lower_0_5 +
                   filled_at_volume_lower_1 +
                   filled_at_volume_lower_5 +
                   filled_at_volume_lower_10 +
                   filled_at_volume_higher_10;
        }

        std::string percent_format(double value)
        {
            return std::to_string(std::ceil(value)) + "%";
        }

        Json data_by_percent()
        {
            return {
                {"<0.5", percent_format(filled_at_volume_lower_0_5 / total() * 100.0)},
                {"<1", percent_format(filled_at_volume_lower_1 / total() * 100.0)},
                {"<5", percent_format(filled_at_volume_lower_5 / total() * 100.0)},
                {"<10", percent_format(filled_at_volume_lower_10 / total() * 100.0)},
                {">10", percent_format(filled_at_volume_higher_10 / total() * 100.0)},
            };
        }

        void clear()
        {
            filled_buy_order_count = 0;
            filled_sell_order_count = 0;
            filled_at_volume_lower_0_5 = 0;
            filled_at_volume_lower_1 = 0;
            filled_at_volume_lower_5 = 0;
            filled_at_volume_lower_10 = 0;
            filled_at_volume_higher_10 = 0;
        }
    };

    double m_inventory = 0.0;
    double m_current_price = 0.0;
    double m_last_quoted_price = 0.0;
    double m_min_trade_volume = 0.0;
    double m_price_gap = 10.0;
    double m_volume = 0.0;
    double m_total_volume_in_usd_in_15_mins = 0.0;
    double m_number_of_order_pair_per_quote = 1.0;
    double m_total_buy_volume = 0.0;
    double m_total_sell_volume = 0.0;
    bool   m_is_closing_far_orders = false;
    FillStat m_fill_stat;
    std::unordered_map<OrderId, Order> m_open_orders;

    // Generate order
    Order get_limit_order(Order::Side side, double price, double quantity);

    void quote_orders_at_price(double price);
    void start_close_far_orders();
    void update_15_mins_volume_stat();
    Task<void> task_close_far_orders();
    Task<void> remove_old_trades();

    void handle_price_update(PriceUpdate price);
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
};
