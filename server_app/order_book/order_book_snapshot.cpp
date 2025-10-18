#include <order_book/order_book_snapshot.h>

void OrderBookSnapShot::update_instrument(const Instrument* instr)
{
    instrument = instr;
}

void OrderBookSnapShot::clear()
{
    bids_size = 0;
    asks_size = 0;
}

void OrderBookSnapShot::print_order_book()
{
    spdlog::info("[{}] asks: ", instrument->symbol);
    for (size_t i = 0; i < asks_size; ++i)
    {
        spdlog::info("[{}] [{} - {}], ", instrument->symbol, asks[i].price, asks[i].quantity);
    }

    spdlog::info("[{}] bids: ", instrument->symbol);
    for (size_t i = 0; i < bids_size; ++i)
    {
        spdlog::info("[{}] [{} - {}], ", instrument->symbol, bids[i].price, bids[i].quantity);
    }
}

double OrderBookSnapShot::get_mid_price()
{
    return (get_best_bid() + get_best_ask()) / 2.0;
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

double OrderBookSnapShot::get_bid_volume()
{
    double volume = 0.0;
    for (size_t i = 0; i < bids_size; ++i)
    {
        volume += bids[i].quantity;
    }
    return volume;
}

double OrderBookSnapShot::get_ask_volume()
{
    double volume = 0.0;
    for (size_t i = 0; i < asks_size; ++i)
    {
        volume += asks[i].quantity;
    }
    return volume;
}