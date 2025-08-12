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
    spdlog::debug("[Rest] OrderBook update snapshot for symbol: {}", instrument->symbol);
    spdlog::debug("[Rest] asks: ");
    for (auto& order_book_level : asks)
    {
        spdlog::debug("[Rest] [{} - {}], ", order_book_level.price, order_book_level.quantity);
    }

    spdlog::debug("[Rest] bids: ");
    for (auto& order_book_level : bids)
    {
        spdlog::debug("[Rest] [{} - {}], ", order_book_level.price, order_book_level.quantity);
    }
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