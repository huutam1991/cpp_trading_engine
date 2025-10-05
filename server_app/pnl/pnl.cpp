#include <pnl/pnl.h>

Json PnL::get_data()
{
    return {
        {"volume", volume},
        {"price_detail", {
            {"price_diff", current_price - avg_price},
            {"avg_price", avg_price},
            {"current_price", current_price}
        }},
        {"realized_detail", {
            {"loss", realized_loss},
            {"profit", realized_profit},
        }},
        {"[realized]", realized},
        {"[unrealized]", unrealized}
    };
}

void PnL::reset()
{
    volume = 0.0;
    avg_price = 0.0;
    current_price = 0.0;
    realized = 0.0;
    realized_profit = 0.0;
    realized_loss = 0.0;
    unrealized = 0.0;
    price_diff = 0.0;
}

void PnL::update_instrument(const Instrument* ins)
{
    instrument = ins;
}

void PnL::update_realized_profit_loss(double value)
{
    if (value > 0.0)
    {
        realized_profit += value;
    }
    else
    {
        realized_loss += value;
    }

    realized += value;
}

void PnL::update_current_price(double price)
{
    current_price = price;
    unrealized = volume * (current_price - avg_price);
}

void PnL::update_trade(double price, double volume, double fee)
{
    if (std::abs(volume) < 1e-12)
    {
        // No trade
        return;
    }
    else if (std::abs(this->volume + volume) < 1e-12)
    {
        // Close all position
        update_realized_profit_loss(this->volume * (price - avg_price) - fee);
        this->volume = 0.0;
        avg_price = 0.0;
        unrealized = 0.0;
    }
    else if (this->volume * volume > 0.0)
    {
        // Increase position
        double total_cost = this->avg_price * this->volume + price * volume + fee;
        this->volume += volume;
        this->avg_price = total_cost / this->volume;
    }
    else
    {
        // Reduce position
        if (std::abs(volume) > std::abs(this->volume))
        {
            // Close all position and open new position
            update_realized_profit_loss(this->volume * (price - avg_price) - fee);
            this->avg_price = price;
            this->volume += volume;
        }
        else
        {
            // Close part of position
            double sign = (this->volume > 0.0) ? 1.0 : -1.0;
            update_realized_profit_loss(sign * std::abs(volume) * (price - avg_price) - fee);
            this->volume += volume;
        }
    }

    unrealized = this->volume * (current_price - avg_price);
}