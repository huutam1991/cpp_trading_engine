#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_run.h>
#include <time/measure_time.h>

StrategyBuySpotStateRun::StrategyBuySpotStateRun(std::shared_ptr<Gateway> gateway, const StrategyBuySpotConfig& config)
    : m_gateway{gateway}, m_config{config}
{
    m_buy_points = SavableObject<BuyPoint>::load_objects_map<double>(m_strategy_buy_spot_db_name, "buy_points", "price");
}

void StrategyBuySpotStateRun::begin()
{
    on_config_change();
    spdlog::debug("StrategyBuySpotStateRun - begin");
}

void StrategyBuySpotStateRun::end()
{
    spdlog::debug("StrategyBuySpotStateRun - end");

    // Send cancel all of placed order
    spdlog::debug("StrategyBuySpotStateRun - cancel all symbol: {}", m_instrument->exchange_symbol);
    m_gateway->cancel_all(m_instrument->exchange_symbol);

    // Update all not HOLD buy points to AVAILABLE
    spdlog::debug("StrategyBuySpotStateRun - update all not HOLD buy points to AVAILABLE or HOLD");
    for (auto& [price, buy_point] : m_buy_points)
    {
        if (buy_point.object.status != BuyPoint::Status::HOLD)
        {
            BuyPoint buy_point_data = buy_point.object;
            buy_point_data.status = buy_point_data.quantity == 0 ? BuyPoint::Status::AVAILABLE : BuyPoint::Status::HOLD;
            buy_point = buy_point_data;   
        }  
    }
}

void StrategyBuySpotStateRun::on_config_change()
{
    m_current_price = 0.0;
    
    // Get new instruments + re-subscribe symbols
    m_instrument = m_gateway->get_instrument_by_symbol(m_config.symbol);
    m_gateway->subscribe_symbol({m_instrument->exchange_symbol});

    spdlog::debug("StrategyBuySpotStateRun, instrument: {}", m_instrument->to_json());
}

void StrategyBuySpotStateRun::add_buy_point_at_price(double price)
{
    m_buy_points.emplace(price, SavableObject<BuyPoint>(
        m_strategy_buy_spot_db_name, 
        "buy_points", 
        BuyPoint {
            price,
            0.0,
            0.0,
            BuyPoint::Status::AVAILABLE
        }
    ));
}

SavableObject<BuyPoint>* StrategyBuySpotStateRun::get_buy_point_by_price(double price)
{
    auto it = m_buy_points.find(price);
    return it == m_buy_points.end() ? nullptr : &it->second;
}

Order StrategyBuySpotStateRun::get_limit_buy_spot_order_by_price(double price)
{
    // // MeasureTime t("get_limit_buy_spot_order_by_price");

    double quantity = m_config.buy_volumn / price;
    double round_up_quantity = m_instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        m_instrument->symbol,
        m_instrument->exchange_symbol,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        round_up_quantity
    );
}

Order StrategyBuySpotStateRun::get_limit_sell_spot_order(double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        m_instrument->symbol,
        m_instrument->exchange_symbol,
        Order::Side::SELL,
        Order::OrderType::LIMIT,
        price,
        quantity
    );
}

Order StrategyBuySpotStateRun::get_cancel_order(OrderId order_id)
{
    return Order(
        order_id,
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        m_instrument->symbol,
        m_instrument->exchange_symbol,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        0.0,
        0.0
    );
}

double StrategyBuySpotStateRun::get_a_price_point()
{
    // If there's no buy point yet, add 1 at [m_current_price]
    if (m_buy_points.size() == 0)
    {
        add_buy_point_at_price(m_current_price - m_config.move_price);
    }

    // Return any price
    double res;
    for (auto& [price, _] : m_buy_points)
    {
        res = price;
        break;
    }

    return res;
}

double StrategyBuySpotStateRun::get_lower_nearest_price()
{
    double price = get_a_price_point();

    while (price >= m_current_price)
    {
        price -= m_config.move_price;
    }

    while (price + m_config.move_price < m_current_price)
    {
        price += m_config.move_price;
    }

    return price;
}

void StrategyBuySpotStateRun::add_new_buy_points()
{
    for (size_t i = 0; i < m_config.max_open_orders; i++)
    {
        double price = m_lower_nearest_price - m_config.move_price * i;
        if (m_buy_points.find(price) == m_buy_points.end())
        {
            add_buy_point_at_price(price);
        }
    }
}

void StrategyBuySpotStateRun::update_buy_orders()
{
    // Check to place buy orders, base on [m_config.max_open_orders]
    for (size_t i = 0; i < m_config.max_open_orders; i++)
    {
        double price = m_lower_nearest_price - m_config.move_price * i;
        auto* buy_point = get_buy_point_by_price(price);
        BuyPoint buy_point_data = buy_point->object;

        if (buy_point_data.status == BuyPoint::Status::AVAILABLE)
        {
            Order order = get_limit_buy_spot_order_by_price(price);
            m_gateway->place_none_wait(order);

            // Update [buy_point]
            buy_point_data.status = BuyPoint::Status::PLACING;
            *buy_point = buy_point_data;
        }
    }

    // Check to cancel buy orders, base on [m_config.max_open_orders]
    double min_price_to_place = m_lower_nearest_price - (m_config.max_open_orders - 1) * m_config.move_price;
    for (auto& [price, buy_point] : m_buy_points)
    {
        BuyPoint buy_point_data = buy_point.object;

        if (price < min_price_to_place && buy_point_data.status == BuyPoint::Status::PLACED && buy_point_data.quantity == 0.0)
        {
            Order order = get_cancel_order(buy_point_data.current_order_id);
            m_gateway->cancel(order);

            // Update [buy_point]
            buy_point_data.status = BuyPoint::Status::CANCELING;
            buy_point = buy_point_data;
        }
    }
}

void StrategyBuySpotStateRun::update_sell_orders()
{
    // Check to get all of HOLD buy points's price
    std::vector<double> hold_buy_points_prices;
    for (auto& [price, buy_point] : m_buy_points)
    {
        if (buy_point.object.status == BuyPoint::Status::HOLD)
        {
            hold_buy_points_prices.push_back(price);        
        }
    }

    // Sort [hold_buy_points_prices] in ascending order
    std::sort(hold_buy_points_prices.begin(), hold_buy_points_prices.end());    

    // Resize [hold_buy_points_prices] to [m_config.max_open_orders]
    if (hold_buy_points_prices.size() > m_config.max_open_orders)
    {
        hold_buy_points_prices.resize(m_config.max_open_orders);    
    }

    // If there's no HOLD buy points, return
    if (hold_buy_points_prices.size() == 0)
    {           
        return;
    }

    double min_hold_price = hold_buy_points_prices[0];
    double max_hold_price = hold_buy_points_prices[hold_buy_points_prices.size() - 1];

    // Check to place sell orders, base on [m_config.max_open_orders]
    for (auto& [price, buy_point] : m_buy_points)
    {
        BuyPoint buy_point_data = buy_point.object;

        // If buy point is HOLD and its price is in range of [min_hold_price, max_hold_price]
        if (buy_point_data.status == BuyPoint::Status::HOLD && price >= min_hold_price && price <= max_hold_price)
        {
            // Calculate profit
            double profit = price - buy_point_data.price;
            if (profit >= m_config.take_profit)
            {
                Order order = get_limit_sell_spot_order(price, buy_point_data.quantity);
                order.side = Order::Side::SELL;
                m_gateway->place_none_wait(order);

                // Update [buy_point]
                buy_point_data.status = BuyPoint::Status::PLACING;
                buy_point = buy_point_data;
            }
        }
    }   
}

TaskVoid StrategyBuySpotStateRun::handle_price_update(PriceUpdate price_update)
{
    m_current_price = price_update.price;
    spdlog::debug("StrategyBuySpotStateRun - m_current_price: {}", m_current_price);
    if (m_current_price >= m_config.max_price || m_current_price <= m_config.min_price)
    {
        spdlog::debug("StrategyBuySpotStateRun - dont handle, min_price: {}, max_price: {}", m_config.min_price, m_config.max_price);
        co_return;
    }

    m_lower_nearest_price = get_lower_nearest_price();

    add_new_buy_points();
    update_buy_orders();
    update_sell_orders();

    co_return;
}

TaskVoid StrategyBuySpotStateRun::handle_order_update(Order& order)
{
    MeasureTime a("StrategyBuySpotStateRun - handle_order_update", MeasureUnit::MICROSECOND);

    double buy_point_price = order.side == Order::Side::BUY ? order.price : order.price - m_config.take_profit;
    SavableObject<BuyPoint>* buy_point = get_buy_point_by_price(buy_point_price);

    // Dont handle invalid order
    if (buy_point == nullptr)
    {
        spdlog::warn("StrategyBuySpotStateRun - handle_order_update - buy point not found for price: {}", buy_point_price);
        co_return;
    }

    // Get [buy_point_data]
    BuyPoint buy_point_data = buy_point->object;

    // NEW - update buy point's status to PLACED
    if (order.status == Order::Status::NEW && order.type == Order::OrderType::LIMIT)
    {
        // Update [buy_point] - set status to PLACED
        buy_point_data.status = BuyPoint::Status::PLACED;
        buy_point_data.current_order_id = order.order_id;
    }
    // FILLED 
    else if (order.status == Order::Status::FILLED)
    {
        if (order.side == Order::Side::BUY)
        {
            // Update [buy_point] - set status to HOLD
            buy_point_data.status = BuyPoint::Status::HOLD;
            buy_point_data.quantity = order.quantity;
            buy_point_data.current_order_id = 0;
            buy_point_data.input = order.volumn_in_quote_currency;
        }
        else if (order.side == Order::Side::SELL)
        {
            // Update [buy_point] - set status to AVAILABLE
            buy_point_data.status = BuyPoint::Status::AVAILABLE;
            buy_point_data.quantity = 0.0;
            buy_point_data.current_order_id = 0;
            buy_point_data.output = order.output_quantity;
            buy_point_data.profit = order.output_quantity - buy_point_data.input;
        }
    }
    // CANCELED | REJECTED - update buy point's status to AVAILABLE
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        // Update [buy_point] - set status to AVAILABLE or HOLD
        buy_point_data.status = buy_point_data.quantity == 0 ? BuyPoint::Status::AVAILABLE : BuyPoint::Status::HOLD;
        buy_point_data.current_order_id = 0;
    }
    
    *buy_point = buy_point_data;

    co_return;
}

TaskVoid StrategyBuySpotStateRun::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        co_await handle_price_update(std::move(price_update));
    }
    else
    {
        Order order = std::get<Order>(data);
        co_await handle_order_update(order);
    }

    co_return;
}

// Json StrategyBuySpotStateRun::get_open_orders()
// {
//     Json open_orders = Json::create_array();

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
