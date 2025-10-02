#pragma once

#include <instrument/instrument.h>

class PnL
{
public:
    const Instrument* instrument = nullptr;
    double volume = 0.0;
    double avg_price = 0.0;
    double current_price = 0.0;
    double realized = 0.0;
    double realized_profit = 0.0;
    double realized_loss = 0.0;
    double unrealized = 0.0;
    double price_diff = 0.0;

    PnL() = default;
    PnL(const Instrument* ins) :
        instrument{ins},
        volume{0.0}, avg_price{0.0},
        current_price{0.0},
        realized{0.0},
        realized_profit{0.0},
        realized_loss{0.0},
        unrealized{0.0},
        price_diff{0.0}
    {}

    Json get_data();
    void reset();
    void update_instrument(const Instrument* ins);
    void update_realized_profit_loss(double value);
    void update_current_price(double price);
    void update_trade(double price, double volume, double fee);
};