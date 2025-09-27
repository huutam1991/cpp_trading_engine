#include <pnl/pnl.h>

void PnL::update_current_price(double price)
{
    current_price = price;
    unrealized = volume * (current_price - avg_price);
}

void PnL::update_trade(double price, double volume, double fee)
{
    if (std::abs(this->volume + volume) < 1e-12)
    {
        // Close all position
        realized += this->volume * (price - avg_price) - fee;
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
            realized += this->volume * (price - avg_price) - fee;
            this->avg_price = price;
            this->volume += volume;
        }
        else
        {
            // Close part of position
            double sign = (this->volume > 0.0) ? 1.0 : -1.0;
            realized += sign * volume * (price - avg_price) - fee;
            this->volume += volume;
        }
    }

    unrealized = this->volume * (current_price - avg_price);
}