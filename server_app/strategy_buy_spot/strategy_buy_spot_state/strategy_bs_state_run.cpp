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
}

void StrategyBuySpotStateRun::on_config_change()
{
    m_current_price = 0.0;
    
    // Get new instruments + re-subscribe symbols
    m_instrument = m_gateway->get_instrument_by_symbol(m_config.symbol);
    m_gateway->subscribe_symbol({m_instrument->exchange_symbol});

    spdlog::debug("StrategyBuySpotStateRun, instrument: {}", m_instrument->to_json());
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

void StrategyBuySpotStateRun::update_buy_points(double price)
{
    // Check to init [m_buy_points]
    if (m_buy_points.size() == 0)
    {
        for (size_t i = 0; i < m_config.max_open_orders; i++)
        {
            price -= m_config.move_price * i;

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
    }
}

void StrategyBuySpotStateRun::remove_open_order_by_price(double price)
{
    
}

void StrategyBuySpotStateRun::check_place_order_at_price(double price)
{
    
}

void StrategyBuySpotStateRun::check_cancel_order_at_price(double price)
{
    
}

void StrategyBuySpotStateRun::update_orders_at_price(double price)
{
    check_place_order_at_price(price);
}

TaskVoid StrategyBuySpotStateRun::handle_price_update(PriceUpdate price_update)
{
    m_current_price = price_update.price;

    if (m_current_price >= m_config.max_price || m_current_price <= m_config.min_price)
    {
        spdlog::debug("StrategyBuySpotStateRun - dont handle price: {}, min_price: {}, max_price: {}", m_current_price, m_config.min_price, m_config.max_price);
        co_return;
    }

    update_buy_points(m_current_price);

    update_orders_at_price(m_current_price);
    check_cancel_order_at_price(m_current_price);

    co_return;
}

TaskVoid StrategyBuySpotStateRun::handle_order_update(Order& order)
{
    // NEW - do nothing
    if (order.status == Order::Status::NEW && order.type == Order::OrderType::LIMIT)
    {
        
    }
    // FILLED - check to continue place chain of orders
    else if (order.status == Order::Status::FILLED)
    {
        
    }
    // CANCELED - remove from [m_current_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        remove_open_order_by_price(order.price);
    }

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
