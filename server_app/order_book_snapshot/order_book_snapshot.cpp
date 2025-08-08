#include <order_book_snapshot/order_book_snapshot.h>

void OrderBookSnapShot::update_instrument(const Instrument* instr)
{
    instrument = instr;
}

void OrderBookSnapShot::reset()
{
    bids.clear();
    asks.clear();
}

void OrderBookSnapShot::add_bid(double price, double quantity)
{
    bids.emplace_back(price, quantity);
}

void OrderBookSnapShot::add_ask(double price, double quantity)
{
    asks.emplace_back(price, quantity);
}

double OrderBookSnapShot::get_max_bid()
{
    return bids[0].price;
}

double OrderBookSnapShot::get_max_ask()
{
    return asks[0].price;
}