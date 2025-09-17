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

void OrderBookSnapShot::print_order_book()
{
    spdlog::info("[{}] asks: ", instrument->symbol);
    for (auto& order_book_level : asks)
    {
        spdlog::info("[{}] [{} - {}], ", instrument->symbol, order_book_level.price, order_book_level.quantity);
    }

    spdlog::info("[{}] bids: ", instrument->symbol);
    for (auto& order_book_level : bids)
    {
        spdlog::info("[{}] [{} - {}], ", instrument->symbol, order_book_level.price, order_book_level.quantity);
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
    for (const auto& level : bids)
    {
        volume += level.quantity;
    }
    return volume;
}

double OrderBookSnapShot::get_ask_volume()
{
    double volume = 0.0;
    for (const auto& level : asks)
    {
        volume += level.quantity;
    }
    return volume;
}