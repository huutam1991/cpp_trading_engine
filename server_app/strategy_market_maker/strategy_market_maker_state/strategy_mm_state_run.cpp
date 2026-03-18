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
    m_start_time = Utils::get_time_now_in_utc_seconds();
    spdlog::info("StrategyMarketMakerStateRun - begin");
}

void StrategyMarketMakerStateRun::end()
{
    spdlog::info("StrategyMarketMakerStateRun - end");

    m_inventory = 0.0;
    m_current_price = 0.0;
    m_last_quoted_price = 0.0;
    m_min_trade_volume = 0.0;
    m_price_gap = 10.0;
    m_volume = 0.0;
    m_total_volume_in_usd_in_15_mins = 0.0;
    m_number_of_order_pair_per_quote = 1.0;
    m_total_buy_volume = 0.0;
    m_total_sell_volume = 0.0;
    m_start_time = 0;
    m_open_orders.clear();
    m_fill_stat.clear();
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
        {"time_running_in_hours", (Utils::get_time_now_in_utc_seconds() - m_start_time) / 3600.0},
        {"fill_volume_stat", {
            {"<0.5", m_fill_stat.filled_at_volume_lower_0_5},
            {"<1", m_fill_stat.filled_at_volume_lower_1},
            {"<5", m_fill_stat.filled_at_volume_lower_5},
            {"<10", m_fill_stat.filled_at_volume_lower_10},
            {">10", m_fill_stat.filled_at_volume_higher_10},
            {"by_percent", m_fill_stat.data_by_percent()}
        }},
        {"orders_count", {
            {"[total_orders_placed]", m_fill_stat.filled_buy_order_count + m_fill_stat.filled_sell_order_count + m_open_orders.size()},
            {"open_orders_count", m_open_orders.size()},
            {"filled_count", {
                {"buy", m_fill_stat.filled_buy_order_count},
                {"sell", m_fill_stat.filled_sell_order_count},
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

void StrategyMarketMakerStateRun::update_15_mins_volume_stat()
{
    m_15_mins_volume_stat.remove_old_volumes(900); // 15 minutes

    if (Utils::get_time_now_in_utc_seconds() - m_start_time > 900)
    {
        Json data = m_15_mins_volume_stat.get_data();
        double total_buy_volume = data["total_buy_volume"];
        double total_sell_volume = data["total_sell_volume"];
        double total_volume_in_15_mins = std::max(total_buy_volume, total_sell_volume);
        m_total_volume_in_usd_in_15_mins = total_volume_in_15_mins * m_current_price;
    }

    if (m_total_volume_in_usd_in_15_mins == 0.0)
    {
        return;
    }

    m_total_volume_in_usd_in_15_mins /= 1000000;

    if (m_total_volume_in_usd_in_15_mins < 15.0) // 10 million USDC
    {
        m_number_of_order_pair_per_quote = 4.0;
    }
    else if (m_total_volume_in_usd_in_15_mins < 25.0) // 20 million USDC
    {
        m_number_of_order_pair_per_quote = 3.0;
    }
    else if (m_total_volume_in_usd_in_15_mins < 35.0) // 30 million USDC
    {
        m_number_of_order_pair_per_quote = 2.0;
    }
    else
    {
        m_number_of_order_pair_per_quote = 1.0;
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

    // Check every 1 seconds
    co_await Timer::sleep_for(1000);
    m_is_closing_far_orders = false;
    start_close_far_orders();

    co_return;
}

Task<void> StrategyMarketMakerStateRun::remove_old_trades()
{
    update_15_mins_volume_stat();
    m_volume_stat.remove_old_volumes(m_config.trade_volume_duration);

    // Check update [m_min_trade_volume]
    Json volume_stat_data = m_volume_stat.get_data();
    m_total_buy_volume = volume_stat_data["total_buy_volume"];
    m_total_sell_volume = volume_stat_data["total_sell_volume"];

    double max_volume = std::max(m_total_buy_volume, m_total_sell_volume);
    double min_volume = std::min(m_total_buy_volume, m_total_sell_volume);
    double volume_ratio = max_volume / min_volume;
    m_min_trade_volume = (max_volume / 100.0) * std::pow(volume_ratio, m_config.min_trade_volume_step) * 0.3;

    // set [m_price_gap]
    if (max_volume < 5.0)
    {
        m_price_gap = 10.0;
    }
    else if (max_volume < 10.0)
    {
        m_price_gap = 15.0;
    }
    else if (max_volume < 20.0)
    {
        m_price_gap = 20.0;
    }
    else if (max_volume < 30.0)
    {
        m_price_gap = 25.0;
    }
    else if (max_volume < 40.0)
    {
        m_price_gap = 30.0;
    }
    else if (max_volume > 40.0)
    {
        m_price_gap = 35.0;
    }
    m_price_gap *= m_config.price_gap;

    // set [m_volume]
    if (max_volume < 0.5)
    {
        m_volume = 0.4;
    }
    else if (max_volume < 1.0)
    {
        m_volume = 0.3;
    }
    else if (max_volume < 5.0)
    {
        m_volume = 0.2;
    }
    else if (max_volume < 10.0)
    {
        // m_volume = 0.15;
        // Hard code
        m_volume = 0.2;
    }
    else if (max_volume < 15.0)
    {
        // m_volume = 0.1;
        // Hard code
        m_volume = 0.2;
    }
    else
    {
        // m_volume = Utils::smooth_curve(max_volume);
        // Hard code
        m_volume = 0.2;
    }

    m_volume *= m_config.volumn;
    m_volume = std::max(m_volume, 0.002);
    m_volume = m_instrument->get_round_up_quantity(m_volume);

    // spdlog::info("[total_buy]: {}, [total_sell]: {}, set [m_volume]: {}, set [m_min_trade_volume]: {}",
    //     m_total_buy_volume, m_total_sell_volume, m_volume, m_min_trade_volume);

    co_return;
}

void StrategyMarketMakerStateRun::quote_orders_at_price(double price)
{
    // MeasureTime t("StrategyMarketMakerStateRun - quote_orders_at_price");

    const TradeVolumeAtPrice* buy_volume = nullptr;
    const TradeVolumeAtPrice* sell_volume = nullptr;

    double new_price_gap = m_price_gap;
    if (m_number_of_order_pair_per_quote == 2)
    {
        new_price_gap = m_price_gap * 2;
    }
    else if (m_number_of_order_pair_per_quote == 3)
    {
        new_price_gap = m_price_gap * 3;
    }
    else if (m_number_of_order_pair_per_quote == 4)
    {
        new_price_gap = m_price_gap * 4;
    }

    double buy_begin = price - 2.0;
    double buy_end = price - new_price_gap;
    double sell_begin = price + 2.0;
    double sell_end = price + new_price_gap;
    double buy_quantity = m_volume;
    double sell_quantity = m_volume;

    double skew_ratio = std::abs(m_total_buy_volume - m_total_sell_volume) / std::max(m_total_buy_volume, m_total_sell_volume);
    if (m_total_buy_volume > m_total_sell_volume)
    {
        buy_begin -= new_price_gap * skew_ratio;
        buy_end -= new_price_gap * skew_ratio;
        sell_end -= (sell_end - sell_begin) * skew_ratio;

        buy_quantity = m_volume * (1.0 - skew_ratio);
        sell_quantity = m_volume * (1.0 + skew_ratio);
    }
    else
    {
        buy_end += (buy_begin - buy_end) * skew_ratio;
        sell_begin += new_price_gap * skew_ratio;
        sell_end += new_price_gap * skew_ratio;

        buy_quantity = m_volume * (1.0 + skew_ratio);
        sell_quantity = m_volume * (1.0 - skew_ratio);
    }

    static std::vector<const TradeVolumeAtPrice*> buy_volumes(10);
    static std::vector<const TradeVolumeAtPrice*> sell_volumes(10);
    buy_volumes.resize(0);
    sell_volumes.resize(0);

    for (double p = buy_begin; p >= buy_end; p -= 1.0)
    {
        buy_volume = m_volume_stat.get_trade_volume_at_price(p);
        if (buy_volume != nullptr && buy_volume->total_buy_volume >= m_min_trade_volume)
        {
            buy_volumes.push_back(buy_volume);
        }
        else
        {
            buy_volume = nullptr;
        }
    }

    for (double p = sell_begin; p <= sell_end; p += 1.0)
    {
        sell_volume = m_volume_stat.get_trade_volume_at_price(p);
        if (sell_volume != nullptr && sell_volume->total_sell_volume >= m_min_trade_volume)
        {
            sell_volumes.push_back(sell_volume);
        }
        else
        {
            sell_volume = nullptr;
        }
    }

    if (buy_volumes.size() == 0 || sell_volumes.size() == 0)
    {
        // spdlog::warn("quote_orders_at_price, cannot find max buy/sell volume in range, skip quoting orders");
        return;
    }

    double min_size = std::min(buy_volumes.size(), sell_volumes.size());
    double number_of_order_pair = std::min(m_number_of_order_pair_per_quote, min_size);

    spdlog::info("");
    spdlog::info("=============================================================================================");
    spdlog::info("Quote orders, price: {}, price gap: {}, new price gap: {}, skew_ratio: {}", price, m_price_gap, new_price_gap, skew_ratio * 100.0);
    spdlog::info("Quote orders, buy  range: [{} - {}]", buy_end, buy_begin);
    spdlog::info("Quote orders, sell range: [{} - {}]", sell_begin, sell_end);
    spdlog::info("Quote orders, total volume (USD) in 15 mins: {}, order pairs (by logic): {}", m_total_volume_in_usd_in_15_mins, m_number_of_order_pair_per_quote);
    spdlog::info("Quote orders, buy size: {}, sell size: {}, order pairs (real): {}", buy_volumes.size(), sell_volumes.size(), number_of_order_pair);
    spdlog::info("Quote orders, pairs:");

    for (size_t i = 0; i < number_of_order_pair; i++)
    {
        spdlog::info("Quote orders, buy  [{}] - price: {}, quantity: {}", i + 1, buy_volumes[i]->price, buy_quantity);
        spdlog::info("Quote orders, sell [{}] - price: {}, quantity: {}", i + 1, sell_volumes[i]->price, sell_quantity);

        Order buy_order  = get_limit_order(Order::Side::BUY, buy_volumes[i]->price, buy_quantity);
        Order sell_order = get_limit_order(Order::Side::SELL, sell_volumes[i]->price, sell_quantity);

        m_gateway->place(buy_order);
        m_gateway->place(sell_order);
    }

    spdlog::info("=============================================================================================");
    spdlog::info("");

    m_last_quoted_price = price;
}

void StrategyMarketMakerStateRun::handle_price_update(PriceUpdate& price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");
    m_pnl.update_current_price(price_update.price);
}

void StrategyMarketMakerStateRun::handle_trade_update(TradeUpdate& trade_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_trade_update");

    std::string side = trade_update.is_buy ? "BUY" : "SELL";
    // spdlog::info("StrategyMarketMakerStateRun: trade update, symbol: {}, side: {}, price: {}, quantity: {}",
    //     trade.instrument->symbol, side, trade.price, trade.quantity);

    m_volume_stat.add_trade_volume(trade_update);
    m_15_mins_volume_stat.add_trade_volume(trade_update);
}

void StrategyMarketMakerStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMarketMakerStateRun - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;

    m_current_price = mid;
    m_pnl.update_current_price(mid);

    // If there's no open orders, quote new orders
    if (m_last_quoted_price == 0.0 ||
        m_open_orders.size() == 0 ||
        std::abs(mid - m_last_quoted_price) >= m_price_gap)
    {
        quote_orders_at_price(mid);
    }
}

void StrategyMarketMakerStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_order_update", MeasureUnit::MICROSECOND);

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

        spdlog::info("*****");
        spdlog::info("***** ORDER FILLED - side: {}, price: {}, quantity: {}",
            enum_reflect::enum_name(order.side),
            order.filled_price, order.filled_quantity
        );
        spdlog::info("***** Volume: {}", m_pnl.volume);
        spdlog::info("*****");

        // 1st order (LIMIT)
        if (order.side == Order::Side::BUY)
        {
            m_inventory += 1;
            m_fill_stat.filled_buy_order_count++;
            m_fill_stat.update(order.filled_quantity);
        }
        else if (order.side == Order::Side::SELL)
        {
            m_inventory -= 1;
            m_fill_stat.filled_sell_order_count++;
            m_fill_stat.update(order.filled_quantity);
        }

        m_open_orders.erase(order.order_id);
    }
    // CANCELED or REJECTED - remove from [m_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        m_open_orders.erase(order.order_id);
    }
}