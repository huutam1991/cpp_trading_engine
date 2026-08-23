#pragma once

#include <stdint.h>
#include <order/order.h>
#include <order_book/order_book_snapshot.h>

struct SpreadCaptureConfig
{
    double move_distance = 2.5; // in USD
    double entry_distance = 1.5; // in USD
    double take_profit = 0.2; // in USD
    double stop_loss = 2.0; // in USD

    double mean_price = 0.0; // in USD, updated in real-time based on price movement
    double volatility = 0.0; // in USD, updated in real-time based on price movement
    double current_move_distance = 0.0; // in USD, updated in real-time based on price movement and volatility
    double current_entry_distance = 0.0; // in USD, updated in real-time based on price movement and volatility
    double current_take_profit = 0.0; // in USD, updated in real-time based on price movement and volatility
    double current_stop_loss = 0.0; // in USD, updated in real-time

    uint64_t success = 0;
    uint64_t fail = 0;

    double profit = 0.0;
    double loss = 0.0;

    Order initial_order = nullptr;
    Order hedge_order = nullptr;

    enum Status
    {
        NONE,
        PLACING_INITIAL_ORDER,
        PLACING_HEDGE_ORDER,
        WAITING_FOR_HEDGE_ORDER_FILLED,
        STOP_LOSS
    };

    Status status = Status::NONE;

    void reset()
    {
        volatility = 0.0;
        mean_price = 0.0;
        current_move_distance = 0.0;
        current_entry_distance = 0.0;
        current_take_profit = 0.0;
        current_stop_loss = 0.0;
        success = 0;
        fail = 0;
        profit = 0.0;
        loss = 0.0;
        initial_order = nullptr;
        hedge_order = nullptr;
        status = Status::NONE;
    }

    std::string win_rate() const
    {
        if (success + fail == 0)
        {
            return "0%";
        }

        double rate = double(success) * 100.0 / double(success + fail);

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << rate;

        return ss.str() + "%";
    }
};

class VolatilityEstimator
{
    std::deque<double> prices;
    // [Tam] - temporarily set window to 5 for testing, will change back to 150 later
    // int window = 150;
    int window = 5;

    double current_price = 0.0;
    double prev_price = 0.0;

public:
    inline void reset()
    {
        prices.clear();
    }

    inline void update(double price)
    {
        prices.push_back(price);

        prev_price = current_price;
        current_price = price;

        if (prices.size() > window)
        {
            prices.pop_front();
        }
    }

    double get_prev_price()
    {
        return prev_price;
    }

    double get_current_price()
    {
        return current_price;
    }

    inline double mean()
    {
        if (prices.size() < window)
        {
            return 0.0;
        }

        double sum = 0;
        for (auto p : prices)
        {
            sum += p;
        }

        return sum / prices.size();
    }

    inline double stddev()
    {
        double m = mean();
        double s = 0;

        for (auto p : prices)
        {
            s += (p - m)*(p - m);
        }

        return sqrt(s / prices.size());
    }

    inline double zscore(double price)
    {
        double vol = stddev();
        if (vol == 0)
        {
            return 0;
        }

        return (price - mean()) / vol;
    }
};

struct SpreadCaptureConfigManager
{
    SpreadCaptureConfig spread_capture;
    VolatilityEstimator volatility_estimator;

    void init_from_config(const SpreadCaptureConfig& config);
    void reset();
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
    Json get_info();
};