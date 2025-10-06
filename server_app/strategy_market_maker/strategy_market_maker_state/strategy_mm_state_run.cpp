#include "strategy_mm_state_run.h"
#include <time/measure_time.h>
#include <time/timer.h>
#include <utils/utils.h>
#include <enum_reflect/enum_reflect.h>
#include <utils/utils.h>

StrategyMarketMakerStateRun::StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config, VolumeStat& volume_stat, PnL& pnl)
    : m_gateway{gateway}, m_config{config}, m_event_base{EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY)}, m_volume_stat{volume_stat}, m_pnl{pnl}
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

    m_inventory = 0.0;
    m_current_price = 0.0;
    m_last_quoted_price = 0.0;
    m_min_trade_volume = 0.0;
    m_volume = 0.0;
    m_filled_buy_order_count = 0;
    m_filled_sell_order_count = 0;
    m_open_orders.clear();
    m_pnl.reset();

    // Send cancel all of placed order
    m_gateway->cancel_all(m_instrument->exchange_symbol);
}

void StrategyMarketMakerStateRun::on_config_change()
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
    m_pnl.update_instrument(m_instrument);
    m_min_trade_volume = m_config.min_trade_volume_step;
    m_volume = m_config.volumn;
    start_close_far_orders();
}

Json StrategyMarketMakerStateRun::get_info()
{
    Json open_orders;

    for (const auto& [order_id, order] : m_open_orders)
    {
        Json data = {
            {"side", enum_reflect::enum_name(order.side)},
            {"price", order.price},
            {"quantity", order.quantity}
        };

        // // price distance
        // double distance = std::abs(order.price - m_current_price);
        // data["distance"] = distance;
        // data["is_far_order"] = (distance > m_config.clear_orders_gap);

        open_orders.push_back(data);
    }

    open_orders.sort([](Json& a, Json& b) -> bool
    {
        return (double)a["price"] > (double)b["price"];
    });

    return {
        {"volume_stat", m_volume_stat.get_data(m_min_trade_volume)},
        {"open_orders", open_orders},
        {"orders_count", {
            {"[total_orders_placed]", m_filled_buy_order_count + m_filled_sell_order_count + m_open_orders.size()},
            {"open_orders_count", m_open_orders.size()},
            {"filled_count", {
                {"buy", m_filled_buy_order_count},
                {"sell", m_filled_sell_order_count},
            }}
        }},
        {"current_price", m_current_price},
        {"inventory", m_inventory},
        {"min_trade_volume", m_min_trade_volume},
        {"pnl", m_pnl.get_data()}
    };
}

Order StrategyMarketMakerStateRun::get_limit_order(Order::Side side, double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        side,
        Order::OrderType::LIMIT,
        m_instrument->get_round_up_price(price),
        quantity
    );
}

void StrategyMarketMakerStateRun::start_close_far_orders()
{
    if (m_is_closing_far_orders == false && m_config.is_running == true)
    {
        auto task = task_close_far_orders();
        task.start_running_on(m_event_base);

        auto remove_old_trades_task = remove_old_trades();
        remove_old_trades_task.start_running_on(m_event_base);
    }
}

Task<void> StrategyMarketMakerStateRun::task_close_far_orders()
{
    // spdlog::warn("task_close_far_orders, m_open_orders size: {}", static_cast<uint64_t>(m_open_orders.size()));
    m_is_closing_far_orders = true;

    for (auto& [order_id, order] : m_open_orders)
    {
        double price_distance = std::abs(order.price - m_current_price);
        // spdlog::warn("task_close_far_orders, order: {}, price_distance: {}, clear_orders_gap: {}", order.to_json(), price_distance, m_config.clear_orders_gap);
        if (price_distance > m_config.clear_orders_gap)
        {
            // spdlog::info("task_close_far_orders, clear_orders_gap: cancel order at price: {}, distance: {}, price: {}", order.price, price_distance, m_current_price);
            m_gateway->cancel(order);
        }
    }

    // Check every 10 seconds
    co_await Timer::sleep_for(10000);
    m_is_closing_far_orders = false;
    start_close_far_orders();

    co_return;
}

Task<void> StrategyMarketMakerStateRun::remove_old_trades()
{
    m_volume_stat.remove_old_volumes(m_config.trade_volume_duration);

    // Check update [m_min_trade_volume]
    Json volume_stat_data = m_volume_stat.get_data();
    double total_buy_volume = volume_stat_data["total_buy_volume"];
    double total_sell_volume = volume_stat_data["total_sell_volume"];

    double max_volume = std::max(total_buy_volume, total_sell_volume);
    double min_volume = std::min(total_buy_volume, total_sell_volume);
    double volume_ratio = max_volume / min_volume;

    m_min_trade_volume = (max_volume / 100.0) * (volume_ratio * volume_ratio) * m_config.min_trade_volume_step;

    // set [m_volume]
    m_volume = Utils::smooth_curve(max_volume) * m_config.volumn;
    m_volume = std::max(m_volume, 0.01);
    m_volume = m_instrument->get_round_up_quantity(m_volume);

    spdlog::info("[total_buy]: {}, [total_sell]: {}, set [m_volume]: {}, set [m_min_trade_volume]: {}",
        total_buy_volume, total_sell_volume, m_volume, m_min_trade_volume);

    co_return;
}

void StrategyMarketMakerStateRun::quote_orders_at_price(double price)
{
    MeasureTime t("StrategyMarketMakerStateRun - quote_orders_at_price");

    const TradeVolumeAtPrice* buy_volume = nullptr;
    const TradeVolumeAtPrice* sell_volume = nullptr;

    for (double p = price - 2.0; p >= price - m_config.price_gap; p -= 1.0)
    {
        buy_volume = m_volume_stat.get_trade_volume_at_price(p);
        if (buy_volume != nullptr && buy_volume->total_buy_volume >= m_min_trade_volume)
        {
            break;
        }
        else
        {
            buy_volume = nullptr;
        }
    }

    for (double p = price + 2.0; p <= price + m_config.price_gap; p += 1.0)
    {
        sell_volume = m_volume_stat.get_trade_volume_at_price(p);
        if (sell_volume != nullptr && sell_volume->total_sell_volume >= m_min_trade_volume)
        {
            break;
        }
        else
        {
            sell_volume = nullptr;
        }
    }

    if (buy_volume == nullptr || sell_volume == nullptr)
    {
        // spdlog::warn("quote_orders_at_price, cannot find max buy/sell volume in range, skip quoting orders");
        return;
    }

    spdlog::info("=============================================================================================");
    spdlog::info("quote_orders_at_price, price: {}", price);
    spdlog::info("quote_orders_at_price, max_buy_volume: price: {}, total_buy_volume: {}", buy_volume->price, buy_volume->total_buy_volume);
    spdlog::info("quote_orders_at_price, max_sell_volume: price: {}, total_sell_volume: {}", sell_volume->price, sell_volume->total_sell_volume);
    spdlog::info("=============================================================================================");
    spdlog::info("");

    Order buy_order  = get_limit_order(Order::Side::BUY, buy_volume->price, m_volume);
    Order sell_order = get_limit_order(Order::Side::SELL, sell_volume->price, m_volume);

    m_gateway->place(buy_order);
    m_gateway->place(sell_order);

    m_last_quoted_price = price;
}

void StrategyMarketMakerStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");
    m_pnl.update_current_price(price_update.price);
}

void StrategyMarketMakerStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;

    m_current_price = mid;
    m_pnl.update_current_price(mid);

    // If there's no open orders, quote new orders
    if (m_last_quoted_price == 0.0 ||
        m_open_orders.size() == 0 ||
        std::abs(mid - m_last_quoted_price) >= m_config.price_gap)
    {
        quote_orders_at_price(mid);
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
        // Update PnL
        double trade_volume = (order.side == Order::Side::BUY) ? order.filled_quantity : -order.filled_quantity;
        m_pnl.update_trade(order.filled_price, trade_volume, order.fee);

        // 1st order (LIMIT)
        if (order.side == Order::Side::BUY)
        {
            m_inventory += 1;
            m_filled_buy_order_count++;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_inventory -= 1;
            m_filled_sell_order_count++;
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
    else if (std::holds_alternative<TradeUpdate>(data))
    {
        TradeUpdate trade = std::get<TradeUpdate>(data);
        std::string side = trade.is_buy ? "BUY" : "SELL";

        // spdlog::info("StrategyMarketMakerStateRun: trade update, symbol: {}, side: {}, price: {}, quantity: {}",
        //     trade.instrument->symbol, side, trade.price, trade.quantity);

        m_volume_stat.add_trade_volume(trade);
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