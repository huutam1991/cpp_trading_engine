#pragma once

#include <instrument/instrument.h>

struct PnL
{
    const Instrument* instrument;
    double volume;
    double avg_price;
    double current_price;
    double realized;
    double unrealized;

    PnL(const Instrument* ins) : instrument{ins}, volume{0.0}, avg_price{0.0}, current_price{0.0}, realized{0.0}, unrealized{0.0} {}

    void update_current_price(double price);
    void update_trade(double price, double volume, double fee);
};