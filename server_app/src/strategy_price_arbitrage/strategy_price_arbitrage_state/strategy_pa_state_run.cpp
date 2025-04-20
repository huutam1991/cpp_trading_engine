#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>

#define PRICE_DELTA 5
#define TOO_LOW_PRICE_DELTA 220
#define TOO_HIGH_PRICE_DELTA 190

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
    std::string symbol = m_config.symbol_1;
    double price = current_price - m_config.buy_at_lower_price;
    double quantity = m_config.buy_volumn / price;
    double round_up_quantity = m_gateway->round_up_quantity("spot", symbol, quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::ExchangeType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

TaskVoid StrategyPriceArbitrageStateRun::handle_price_update(double price)
{
    // Place new order if current price is moving > PRICE_DELTA compare to current order's price
    if (std::abs(m_current_order.price - (price - m_config.buy_at_lower_price)) > PRICE_DELTA)
    {
        Order order = get_limit_buy_spot_order_by_price(price);
        m_current_order = co_await m_gateway->place(order, Order::Status::NEW);

        // Insert to [m_current_open_orders]
        m_current_open_orders.insert(std::make_pair(m_current_order.order_id, m_current_order));
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
    // NEW - do nothing
    if (order.status == Order::Status::NEW)
    {}
    // FILLED - update data to order's checkpoint
    else if (order.status == Order::Status::FILLED)
    {
        // TBD
    }
    // CANCELED - update [order_id] = 0 for order's checkpoint
    else if (order.status == Order::Status::CANCELED)
    {
        remove_open_order_id(order.order_id);
    }

    co_return;
}

TaskVoid StrategyPriceArbitrageStateRun::run(StrategyData data)
{
    ADD_LOG("StrategyPriceArbitrageStateRun - run");

    double price;
    if (std::holds_alternative<double>(data))
    {
        double price = std::get<double>(data);
        co_await handle_price_update(price);
    }
    else
    {
        Order order = std::get<Order>(data);
        co_await handle_order_update(order);
    }

    co_return;
}
