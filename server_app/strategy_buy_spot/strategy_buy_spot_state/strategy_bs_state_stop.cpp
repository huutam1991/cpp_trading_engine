#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_stop.h>

StrategyBuySpotStateStop::StrategyBuySpotStateStop()
{
}

void StrategyBuySpotStateStop::begin()
{
    spdlog::info("StrategyBuySpotStateStop - begin");
}

void StrategyBuySpotStateStop::end()
{
    spdlog::info("StrategyBuySpotStateStop - end");
}

void StrategyBuySpotStateStop::handle_price_update(PriceUpdate& price_update)
{
    spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, price: {} ", price_update.instrument->symbol, price_update.price);
}

void StrategyBuySpotStateStop::handle_trade_update(TradeUpdate& trade_update)
{
    spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, price: {}, quantity: {}, is_buy: {}",
        trade_update.instrument->symbol, trade_update.price, trade_update.quantity, trade_update.is_buy);
}

void StrategyBuySpotStateStop::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    double bid_price = snapshot->get_best_bid();
    double ask_price = snapshot->get_best_ask();
    double ask_quantity = snapshot->get_best_ask_quantity();
    double bid_quantity = snapshot->get_best_bid_quantity();

    spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, bid_price: {}, ask_price: {}", snapshot->instrument->symbol, bid_price, ask_price);
    spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, bid_quantity: {}, ask_quantity: {}", snapshot->instrument->symbol, bid_quantity, ask_quantity);
}

void StrategyBuySpotStateStop::handle_order_update(Order& order)
{
    spdlog::info("StrategyBuySpotStateStop: do nothing, symbol: {}, side: {}, price: {}, quantity: {}, status: {}",
        order.instrument->symbol, enum_reflect::enum_name(order.side), order.price, order.quantity, enum_reflect::enum_name(order.status));
}
