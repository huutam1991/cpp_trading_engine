#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_run.h>
#include <time/measure_time.h>

StrategyPriceArbitrageStateRun::StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway> gateway, const StrategyPriceArbitrageConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyPriceArbitrageStateRun::begin()
{
    on_config_change();
    spdlog::debug("StrategyPriceArbitrageStateRun - begin");
}

void StrategyPriceArbitrageStateRun::end()
{
    spdlog::debug("StrategyPriceArbitrageStateRun - end");
    spdlog::debug("cancel all symbol: {}", m_instrument_1->exchange_symbol);

    // Send cancel all of placed order
    m_gateway->cancel_all(m_instrument_1->exchange_symbol);
    m_current_open_orders.clear();
}

void StrategyPriceArbitrageStateRun::on_config_change()
{
    m_current_price = 0.0;

    // Re-subscribe symbols
    // auto ins1 = m_gateway->get_instrument_by_symbol(m_config.symbol_1);
    // auto ins2 = m_gateway->get_instrument_by_symbol(m_config.symbol_2);
    Instrument* ins1 = nullptr;
    Instrument* ins2 = nullptr;
    m_gateway->subscribe_instruments({ins1, ins2});

    // Get new instruments
    // m_instrument_1 = m_gateway->get_instrument_by_symbol(m_config.symbol_1);
    // m_instrument_2 = m_gateway->get_instrument_by_symbol(m_config.symbol_2);
    // m_instrument_3 = m_gateway->get_instrument_by_symbol(m_config.symbol_3);

    spdlog::debug("StrategyPriceArbitrageStateRun, instrument 1: {}", m_instrument_1->to_json());
    spdlog::debug("StrategyPriceArbitrageStateRun, instrument 2: {}", m_instrument_2->to_json());
    spdlog::debug("StrategyPriceArbitrageStateRun, instrument 3: {}", m_instrument_3->to_json());
}

Order StrategyPriceArbitrageStateRun::get_limit_buy_spot_order_by_price(double price)
{
    // // MeasureTime t("get_limit_buy_spot_order_by_price");

    double quantity = m_config.buy_volumn / price;
    double round_up_quantity = m_instrument_1->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument_1,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

Order StrategyPriceArbitrageStateRun::get_market_buy_spot_order_by_symbol_and_quantity(Instrument* instrument, double quantity)
{
    double round_up_quantity = instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        instrument,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyPriceArbitrageStateRun::get_market_sell_spot_order_by_symbol_and_quantity(Instrument* instrument, double quantity)
{
    double round_up_quantity = instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        instrument,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

void StrategyPriceArbitrageStateRun::remove_open_order_by_price(double price)
{
    if (m_current_open_orders.find(price) != m_current_open_orders.end())
    {
        m_current_open_orders.erase(price);
    }
}

void StrategyPriceArbitrageStateRun::check_place_order_at_price(double price)
{
    // Only place 1 order at a time, and dont place new LIMIT order when chain orders is placing
    if (is_placing_chain_orders == true || m_current_open_orders.size() > 0)
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

void StrategyPriceArbitrageStateRun::check_cancel_order_at_price(double price)
{
    // Cancel all of orders that price is too low or too high
    for (auto& [order_price, order_info] : m_current_open_orders)
    {
        if (order_info.is_handeling == true)
        {
            continue;
        }

        if (order_price <= price - m_config.too_low_price_delta || order_price >= price - m_config.too_high_price_delta)
        {
            order_info.is_handeling = true;
            m_gateway->cancel(order_info.order);
        }
    }
}

void StrategyPriceArbitrageStateRun::update_orders_at_price(double price)
{
    check_place_order_at_price(price - m_config.buy_at_lower_price);
}

Task<void> StrategyPriceArbitrageStateRun::handle_price_update(PriceUpdate price_update)
{
    if (price_update.instrument == m_instrument_2)
    {
        m_symbol_2_price = price_update.price;
        co_return;
    }

    double price = price_update.price;
    m_current_price = m_current_price == 0.0 ? price : m_current_price;

    // // Place new order if current price is moving a PRICE_DELTA compare to current price
    // if (price <= m_current_price - m_config.price_delta)
    // {
    //     while (price <= m_current_price - m_config.price_delta)
    //     {
    //         m_current_price -= m_config.price_delta;
    //         update_orders_at_price(m_current_price);
    //     }
    // }
    // else if (price >= m_current_price + m_config.price_delta)
    // {
    //     while (price >= m_current_price + m_config.price_delta)
    //     {
    //         m_current_price += m_config.price_delta;
    //         update_orders_at_price(m_current_price);
    //     }
    // }

    update_orders_at_price(price);
    check_cancel_order_at_price(price);

    co_return;
}

Task<void> StrategyPriceArbitrageStateRun::handle_order_update(Order& order)
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
        // 1st order (LIMIT)
        if (order.type == Order::OrderType::LIMIT)
        {
            // Buy symbol 2 from symbol 1
            double quantity = order.output_quantity / m_symbol_2_price;
            Order order_2 = get_market_buy_spot_order_by_symbol_and_quantity(m_instrument_2, quantity);
            m_gateway->place(order_2);

            remove_open_order_by_price(order.price);

            // Mark [is_placing_chain_orders] to true, no new LIMIT order will be placed until the chain orders is finished
            is_placing_chain_orders = true;
        }
        // 2nd order (MARKET)
        else if (order.type == Order::OrderType::MARKET && order.instrument->exchange_symbol == m_instrument_2->exchange_symbol)
        {
            // Sell symbol 3 from symbol 2
            double quantity = order.output_quantity;
            Order order_3 = get_market_sell_spot_order_by_symbol_and_quantity(m_instrument_3, quantity);
            m_gateway->place(order_3);
        }
        // 3rd order (MARKET)
        else if (order.type == Order::OrderType::MARKET && order.instrument->exchange_symbol == m_instrument_3->exchange_symbol)
        {
            is_placing_chain_orders = false;
        }
    }
    // CANCELED - remove from [m_current_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        remove_open_order_by_price(order.price);
    }

    co_return;
}

Task<void> StrategyPriceArbitrageStateRun::update(StrategyUpdateData data)
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

// Json StrategyPriceArbitrageStateRun::get_open_orders()
// {
//     Json open_orders;

//     for (auto& [_, order_info] : m_current_open_orders)
//     {
//         open_orders.push_back(order_info.order.to_json());
//     }

//     return {
//         {"current_price", m_current_price},
//         {"too_low_price", m_current_price - m_config.too_low_price_delta},
//         {"too_high_price", m_current_price - m_config.too_high_price_delta},
//         {"order", open_orders}
//     };
// }
