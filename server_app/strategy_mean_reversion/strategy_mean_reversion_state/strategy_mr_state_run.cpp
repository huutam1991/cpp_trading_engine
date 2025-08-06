#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <time/measure_time.h>

StrategyMeanReversionStateRun::StrategyMeanReversionStateRun(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config)
    : StrategyMeanReversionState(gateway, config)
{
}

void StrategyMeanReversionStateRun::begin()
{
    m_current_price = 0.0;
    spdlog::info("StrategyMeanReversionStateRun - begin");
}

void StrategyMeanReversionStateRun::end()
{
    spdlog::info("StrategyMeanReversionStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_config.symbol);
    m_current_open_orders.clear();
    is_taking_profit = false;
}

Order StrategyMeanReversionStateRun::get_limit_buy_spot_order_by_price(double price)
{
    // // MeasureTime t("get_limit_buy_spot_order_by_price");
    // Instrument* instrument = m_gateway->get_instrument_by_symbol(m_config.symbol);
    Instrument* instrument = nullptr;

    double quantity = m_config.buy_volumn / price;
    double round_up_quantity = instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        instrument,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

Order StrategyMeanReversionStateRun::get_limit_sell_spot_order_by_price_and_quantity(double price, double quantity)
{
    // Instrument* instrument = m_gateway->get_instrument_by_symbol(m_config.symbol);
    Instrument* instrument = nullptr;
    double round_up_quantity = instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        instrument,
        Order::Side::SELL,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

void StrategyMeanReversionStateRun::remove_open_order_by_price(double price)
{
    if (m_current_open_orders.find(price) != m_current_open_orders.end())
    {
        m_current_open_orders.erase(price);
    }
}

void StrategyMeanReversionStateRun::check_place_order_at_price(double price)
{
    // Only place 1 order at a time, and dont place new LIMIT order when chain orders is placing
    if (is_taking_profit == true || m_current_open_orders.size() > 0)
    {
        return;
    }

    if (m_current_open_orders.find(price) == m_current_open_orders.end())
    {
        Order order = get_limit_buy_spot_order_by_price(price);
        m_gateway->place(order);

        // Insert to [m_current_open_orders]
        m_current_open_orders.insert(std::make_pair(price, OrderInfo{std::move(order), true}));
    }
}

void StrategyMeanReversionStateRun::check_cancel_order_at_price(double price)
{
    // Dont cancel taking profit order
    if (is_taking_profit == true)
    {
        return;
    }

    // Cancel all of orders that price is too low or too high
    for (auto& [order_price, order_info] : m_current_open_orders)
    {
        if (order_info.is_handeling == true)
        {
            continue;
        }

        double lower_price = price - m_config.buy_at_lower_price;
        if (order_price <= lower_price - m_config.too_low_price_delta || order_price >= lower_price + m_config.too_high_price_delta)
        {
            order_info.is_handeling = true;
            m_gateway->cancel(order_info.order);
        }
    }
}

Task<void> StrategyMeanReversionStateRun::handle_price_update(MRPriceUpdate price_update)
{
    m_current_price = price_update.price;

    check_place_order_at_price(m_current_price - m_config.buy_at_lower_price);
    check_cancel_order_at_price(m_current_price);

    co_return;
}

Task<void> StrategyMeanReversionStateRun::handle_order_update(Order& order)
{
    // NEW - do nothing
    if (order.status == Order::Status::NEW && order.type == Order::OrderType::LIMIT)
    {
        if (m_current_open_orders.find(order.price) != m_current_open_orders.end())
        {
            OrderInfo& order_info = m_current_open_orders[order.price];
            order_info.is_handeling = false;
            order_info.order = order;
        }
        else
        {
            // Insert to [m_current_open_orders]
            m_current_open_orders.insert(std::make_pair(order.price, OrderInfo{order, false}));
        }
    }
    // FILLED - check to continue place chain of orders
    else if (order.status == Order::Status::FILLED)
    {
        // 1st order (BUY)
        if (order.side == Order::Side::BUY)
        {
            // Buy symbol 2 from symbol 1
            double quantity = order.output_quantity;
            double price = order.price + m_config.sell_at_higher_price;
            Order order_2 = get_limit_sell_spot_order_by_price_and_quantity(price, quantity);
            m_gateway->place(order_2);

            remove_open_order_by_price(order.price);
            is_taking_profit = true;
        }
        else if (order.side == Order::Side::SELL)
        {
            remove_open_order_by_price(order.price);
            is_taking_profit = false;
        }
    }
    // CANCELED - remove from [m_current_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        remove_open_order_by_price(order.price);
    }

    co_return;
}

Task<void> StrategyMeanReversionStateRun::run(StrategyMeanReversionData data)
{
    MRPriceUpdate price_update;
    if (std::holds_alternative<MRPriceUpdate>(data))
    {
        price_update = std::get<MRPriceUpdate>(data);
        co_await handle_price_update(std::move(price_update));
    }
    else
    {
        Order order = std::get<Order>(data);
        co_await handle_order_update(order);
    }

    co_return;
}

Json StrategyMeanReversionStateRun::get_open_orders()
{
    Json open_orders;

    for (auto& [_, order_info] : m_current_open_orders)
    {
        open_orders.push_back(order_info.order.to_json());
    }

    return {
        {"current_price", m_current_price},
        {"too_low_price", m_current_price - m_config.buy_at_lower_price - m_config.too_low_price_delta},
        {"too_high_price", m_current_price - m_config.buy_at_lower_price + m_config.too_high_price_delta},
        {"order", open_orders}
    };
}
