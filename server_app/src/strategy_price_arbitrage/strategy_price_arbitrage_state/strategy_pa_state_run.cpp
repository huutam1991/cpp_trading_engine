#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>
#include <measure_time.h>

#define PRICE_DELTA 10
#define TOO_LOW_PRICE_DELTA 100
#define TOO_HIGH_PRICE_DELTA 50

StrategyPriceArbitrageStateRun::StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config)
    : StrategyPriceArbitrageState(gateway, config)
{
}

void StrategyPriceArbitrageStateRun::begin()
{
    ADD_LOG("StrategyPriceArbitrageStateRun - begin");
}

void StrategyPriceArbitrageStateRun::end()
{
    ADD_LOG("StrategyPriceArbitrageStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_config.symbol_1);
}

void StrategyPriceArbitrageStateRun::remove_open_order_id(OrderId order_id)
{
    if (m_current_open_orders.find(order_id) != m_current_open_orders.end())
    {
        m_current_open_orders.erase(order_id);
    }
}

Order StrategyPriceArbitrageStateRun::get_limit_buy_spot_order_by_price(double current_price)
{
    // // MeasureTime t("get_limit_buy_spot_order_by_price");

    double price = current_price - m_config.buy_at_lower_price;
    double quantity = m_config.buy_volumn / price;
    double round_up_quantity = m_gateway->round_up_quantity("spot", m_config.symbol_1, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::ExchangeType::SPOT,
        Order::Status::NOT_AVAILABLE,
        m_config.symbol_1,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

Order StrategyPriceArbitrageStateRun::get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::ExchangeType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyPriceArbitrageStateRun::get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::ExchangeType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

TaskVoid StrategyPriceArbitrageStateRun::handle_price_update(PriceUpdate price_update)
{
    if (price_update.symbol == m_config.symbol_2)
    {
        m_symbol_2_price = price_update.price;
        co_return;
    }

    double price = price_update.price;

    // Place new order if current price is moving > PRICE_DELTA compare to current order's price
    if (std::abs(m_current_order.price - (price - m_config.buy_at_lower_price)) > PRICE_DELTA)
    {
        Order order = get_limit_buy_spot_order_by_price(price);
        m_gateway->place(order);
    }

    // Cancel all of orders that price is too low or too high
    for (auto& [order_id, order] : m_current_open_orders)
    {
        if (order.price <= price - TOO_LOW_PRICE_DELTA || order.price >= price - TOO_HIGH_PRICE_DELTA)
        {
            m_gateway->cancel(order);
        }
    }

    co_return;
}

TaskVoid StrategyPriceArbitrageStateRun::handle_order_update(Order& order)
{
    // NEW - insert to [m_current_open_orders]
    if (order.status == Order::Status::NEW)
    {
        m_current_open_orders.insert(std::make_pair(order.order_id, order));
    }
    // FILLED - check to continue place chain of orders
    else if (order.status == Order::Status::FILLED)
    {
        // 1st order (LIMIT)
        if (order.type == Order::OrderType::LIMIT)
        {
            // Buy symbol 2 from symbol 1
            double quantity = order.output_quantity / m_symbol_2_price;
            Order order_2 = get_market_buy_spot_order_by_symbol_and_quantity(m_config.symbol_2, quantity);
            m_gateway->place(order_2);

            remove_open_order_id(order.order_id);
        }
        // 2nd order (MARKET)
        else if (order.type == Order::OrderType::MARKET && order.symbol == m_config.symbol_2)
        {
            // Sell symbol 3 from symbol 2
            double quantity = order.output_quantity;
            Order order_3 = get_market_sell_spot_order_by_symbol_and_quantity(m_config.symbol_3, quantity);
            m_gateway->place(order_3);

            remove_open_order_id(order.order_id);
        }
        // 3rd order (MARKET)
        else if (order.type == Order::OrderType::MARKET && order.symbol == m_config.symbol_3)
        {
            remove_open_order_id(order.order_id);
        }
    }
    // CANCELED - remove from [m_current_open_orders]
    else if (order.status == Order::Status::CANCELED)
    {
        remove_open_order_id(order.order_id);
    }

    co_return;
}

TaskVoid StrategyPriceArbitrageStateRun::run(StrategyPriceArbitrageData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        co_await handle_price_update(price_update);
    }
    else
    {
        Order order = std::get<Order>(data);
        co_await handle_order_update(order);
    }

    co_return;
}
