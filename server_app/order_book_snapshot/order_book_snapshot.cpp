#include <order_book_snapshot/order_book_snapshot.h>


double OrderbookSnapShot::get_max_bid()
{
    return m_bids[0].price;
}

double OrderbookSnapShot::get_max_ask()
{
    return m_asks[0].price;
}