#include <order_book/order_book_snapshot.h>

void OrderBookSnapShot::update_instrument(const Instrument* instr)
{
    instrument = instr;
}

void OrderBookSnapShot::clear()
{
    bids.clear();
    asks.clear();
}

double OrderBookSnapShot::get_best_bid()
{
    return bids[0].price;
}

double OrderBookSnapShot::get_best_ask()
{
    return asks[0].price;
}

double OrderBookSnapShot::get_best_bid_quantity()
{
    return bids[0].quantity;
}

double OrderBookSnapShot::get_best_ask_quantity()
{
    return asks[0].quantity;
}