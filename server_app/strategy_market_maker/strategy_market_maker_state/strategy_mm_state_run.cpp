#include "strategy_mm_state_run.h"
#include <time/measure_time.h>
#include <utils/utils.h>
#include <enum_reflect/enum_reflect.h>

StrategyMarketMakerStateRun::StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config)
    : m_gateway{gateway}, m_config{config}, m_event_base{EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY)}
{
}

void StrategyMarketMakerStateRun::begin()
{
    on_config_change();
    spdlog::info("StrategyMarketMakerStateRun - begin");
}

void StrategyMarketMakerStateRun::end()
{
    spdlog::info("StrategyMarketMakerStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_instrument->exchange_symbol);
}

void StrategyMarketMakerStateRun::on_config_change()
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
}

Json StrategyMarketMakerStateRun::get_info()
{
    Json open_orders;

    for (const auto& [order_id, order] : m_open_orders)
    {
        Json data = order.to_json();
        double distance = std::abs(order.price - m_last_quote_price);
        data["distance"] = distance;
        data["is_far_order"] = (distance > m_config.price_step_between_blocks);
        open_orders.push_back(data);
    }

    return {
        {"inventory", m_inventory},
        {"last_quote_price", m_last_quote_price},
        {"open_orders", open_orders}
    };
}

Order StrategyMarketMakerStateRun::get_buy_limit_order(double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        m_instrument->get_round_up_price(price),
        quantity
    );
}

Order StrategyMarketMakerStateRun::get_sell_limit_order(double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::SELL,
        Order::OrderType::LIMIT,
        m_instrument->get_round_up_price(price),
        quantity
    );
}

Order StrategyMarketMakerStateRun::get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = m_instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyMarketMakerStateRun::get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = m_instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

void StrategyMarketMakerStateRun::close_far_orders(double price)
{
    auto task = task_close_far_orders(price);
    task.start_running_on(m_event_base);
}

Task<void> StrategyMarketMakerStateRun::task_close_far_orders(double price)
{
    spdlog::warn("m_open_orders size: {}", static_cast<uint64_t>(m_open_orders.size()));
    for (auto& [order_id, order] : m_open_orders)
    {
        double price_distance = std::abs(order.price - price);
        spdlog::warn("order: {}, price_distance: {}, price_step_between_blocks: {}", order.to_json(), price_distance, m_config.price_step_between_blocks);
        if (price_distance > m_config.price_step_between_blocks)
        {
            spdlog::info("StrategyMarketMakerStateRun - close_far_orders: cancel order at price: {}, distance: {}, price: {}", order.price, price_distance, price);
            m_gateway->cancel(order);
        }
    }

    co_return;
}

void StrategyMarketMakerStateRun::quote_block_orders_at_price(double price)
{
    // MeasureTime t("StrategyMarketMakerStateRun - quote_block", MeasureUnit::MICROSECOND);

    // // If PAUSE: do not place new orders (keep existing far orders)
    // if (m_mode == Mode::PAUSE)
    // {
    //     spdlog::info("MM quote paused due to high momentum at price {}", price);
    //     return;
    // }

    // // Inventory skew calculation: KEEP the original logic
    // double inventory_in_blocks = std::abs(m_inventory / m_config.orders_each_side_per_block);
    // double alpha_inv = (inventory_in_blocks >= m_config.inventory_skew_ratio) ? 1.0 : 0.0;

    // double widen   = 1.0 + (m_config.widen - 1.0) * alpha_inv;
    // double tighten = 1.0 - (1.0 - m_config.tight) * alpha_inv;

    // double bid_price_gap = m_config.price_gap;
    // double ask_price_gap = m_config.price_gap;
    // if (m_inventory > 0)
    // {
    //     bid_price_gap = m_config.price_gap * widen;
    //     ask_price_gap = m_config.price_gap * tighten;
    // }
    // else if (m_inventory < 0)
    // {
    //     bid_price_gap = m_config.price_gap * tighten;
    //     ask_price_gap = m_config.price_gap * widen;
    // }

    // // If REDUCE: widen both sides equally to reduce fill-rate (keep skew philosophy unchanged)
    // if (m_mode == Mode::REDUCE)
    // {
    //     double mom_mult = (m_config.mom_widen_mult > 1.0) ? m_config.mom_widen_mult : 1.6;
    //     spdlog::info("StrategyMarketMakerStateRun - quote_block_orders_at_price: applying momentum multiplier: {}", mom_mult);
    //     bid_price_gap *= mom_mult;
    //     ask_price_gap *= mom_mult;
    // }

    // spdlog::info("StrategyMarketMakerStateRun - quote_block_orders_at_price: price: {}, bid_gap: {}, ask_gap: {}, inventory: {}, mode: {}",
    //             price, bid_price_gap, ask_price_gap, m_inventory, enum_reflect::enum_name(m_mode));

    // Place orders (respect the max distance per block as you currently do)
    double num_of_limit_orders = m_config.inventory_skew_ratio * m_config.orders_each_side_per_block;
    for (size_t i = 1; i <= m_config.orders_each_side_per_block; i++)
    {
        double buy_price  = price - (i * m_config.price_gap);
        if (std::abs(buy_price - price) <= m_config.price_step_between_blocks)
        {
            // Stop quoting bid if inventory skew ratio is exceeded
            if (m_inventory > 0 && m_inventory + i > num_of_limit_orders)
            {
                spdlog::info("StrategyMarketMakerStateRun - stop quoting buy orders at price: {}, m_inventory + i: {}, num_of_limit_orders: {}", buy_price, m_inventory + i, num_of_limit_orders);
                break;
            }

            Order buy_order = get_buy_limit_order(buy_price, m_config.volumn);
            m_gateway->place(buy_order);
            spdlog::info("Placing buy order: {}", buy_order.to_json());
        }
    }

    for (size_t i = 1; i <= m_config.orders_each_side_per_block; i++)
    {
        double sell_price = price + (i * m_config.price_gap);
        if (std::abs(sell_price - price) <= m_config.price_step_between_blocks)
        {
            // Stop quoting ask if inventory skew ratio is exceeded
            if (m_inventory < 0 && -m_inventory + i > num_of_limit_orders)
            {
                spdlog::info("StrategyMarketMakerStateRun - stop quoting sell orders at price: {}, -m_inventory + i: {}, num_of_limit_orders: {}", sell_price, -m_inventory + i, num_of_limit_orders);
                break;
            }

            Order sell_order = get_sell_limit_order(sell_price, m_config.volumn);
            m_gateway->place(sell_order);
            spdlog::info("Placing sell order: {}", sell_order.to_json());
        }
    }
}

void StrategyMarketMakerStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");
}

void StrategyMarketMakerStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;

    // --- Momentum guard (EMA return z-score) ---
    // if (m_has_last_mid)
    // {
    //     // return r_t ~ (p_t - p_{t-1}) / p_{t-1}
    //     double r = (mid - m_last_mid) / std::max(1e-9, m_last_mid);

    //     // EMA alpha: use config if available, default to 0.2
    //     double alpha = (m_config.mom_window_alpha > 0.0 && m_config.mom_window_alpha <= 1.0)
    //                      ? m_config.mom_window_alpha : 0.2;

    //     // Update EMA mean/variance
    //     m_r_mean = (1 - alpha) * m_r_mean + alpha * r;
    //     double dev = r - m_r_mean;
    //     m_r_var  = (1 - alpha) * m_r_var  + alpha * (dev * dev);
    //     double z = dev / std::sqrt(m_r_var + 1e-12);

    //     // Thresholds (use config if available)
    //     double z_reduce = (m_config.mom_z_reduce > 0.0) ? m_config.mom_z_reduce : 3.0;
    //     double z_pause  = (m_config.mom_z_pause  > 0.0) ? m_config.mom_z_pause  : 5.0;

    //     // Decide mode
    //     if (std::abs(z) >= z_pause)        m_mode = Mode::PAUSE;
    //     else if (std::abs(z) >= z_reduce)  m_mode = Mode::REDUCE;
    //     else                               m_mode = Mode::NORMAL;

    //     spdlog::info("StrategyMarketMakerStateRun - handle_order_book_snapshot: mid: {}, r: {}, z: {}, mode: {}",
    //                 mid, r, z, enum_reflect::enum_name(m_mode));
    // }

    // m_last_mid = mid;
    // m_has_last_mid = true;
    // // --- END momentum guard ---

    if (std::abs(mid - m_last_quote_price) > m_config.price_step_between_blocks)
    {
        quote_block_orders_at_price(mid);
        close_far_orders(mid);
        m_last_quote_price = mid;
    }
}


void StrategyMarketMakerStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_order_update");

    // NEW - add to [m_open_orders]
    if (order.status == Order::Status::NEW)
    {
        m_open_orders.emplace(order.order_id, order);
    }
    // FILLED - update [m_inventory] and remove order from [m_open_orders]
    else if (order.status == Order::Status::FILLED)
    {
        // 1st order (LIMIT)
        if (order.side == Order::Side::BUY)
        {
            m_inventory += 1;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_inventory -= 1;
        }

        m_open_orders.erase(order.order_id);
    }
    // CANCELED or REJECTED - remove from [m_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        m_open_orders.erase(order.order_id);
    }
}

Task<void> StrategyMarketMakerStateRun::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        handle_price_update(price_update);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        handle_order_book_snapshot(snapshot);

        OrderBookSnapShotPool::release(snapshot);
    }
    else
    {
        Order order = std::get<Order>(data);
        handle_order_update(order);
    }

    co_return;
}