#include "strategy_mm_state_run.h"
#include <time/measure_time.h>
#include <utils/utils.h>

StrategyMarketMakerStateRun::StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, StrategyMarketMakerConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyMarketMakerStateRun::begin()
{
    on_config_change();
    ADD_LOG("StrategyMarketMakerStateRun - begin");
}

void StrategyMarketMakerStateRun::end()
{
    ADD_LOG("StrategyMarketMakerStateRun - end");

    m_is_placing = false;

    // Send cancel all of placed order
    m_gateway->cancel_all(m_config.symbol);
}

void StrategyMarketMakerStateRun::on_config_change()
{
    m_current_price = 0.0;
    update_lot_size();
    m_gateway->subscribe_symbol({m_config.symbol});
}

void StrategyMarketMakerStateRun::update_lot_size()
{
    m_lot_size = m_gateway->get_lot_size("spot", m_config.symbol);
    spdlog::debug("StrategyMarketMakerStateRun - update [m_lot_size] = {}", m_lot_size);
}

double StrategyMarketMakerStateRun::local_round_up_quantity(double quantity)
{
    std::string round_str_number = Utils::round_string_number(std::to_string(quantity), m_lot_size);

    return std::stod(round_str_number);
}

Order StrategyMarketMakerStateRun::get_limit_buy_spot_order(double price, double quantity)
{
    // MeasureTime t("get_limit_buy_spot_order");

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        m_config.symbol,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        quantity
    );
}

Order StrategyMarketMakerStateRun::get_limit_sell_spot_order(double price, double quantity)
{
    // MeasureTime t("get_limit_sell_spot_order");

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        m_config.symbol,
        Order::Side::SELL,
        Order::OrderType::LIMIT,
        price,
        quantity
    );
}

Order StrategyMarketMakerStateRun::get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = local_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyMarketMakerStateRun::get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = local_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        InstrumentType::SPOT,
        Order::Status::NOT_AVAILABLE,
        symbol,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

TaskVoid StrategyMarketMakerStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");

    m_current_price = price_update.price;

    if (m_is_placing == false)
    {
        m_is_placing = true;

        double buy_price = m_current_price - m_config.spread / 2;
        double sell_price = m_current_price + m_config.spread / 2;
        double quantity = local_round_up_quantity(m_config.buy_volumn / buy_price);

        m_current_order_buy = get_limit_buy_spot_order(buy_price, quantity);
        m_current_order_sell = get_limit_sell_spot_order(sell_price, quantity);

        m_gateway->place_none_wait(m_current_order_buy);
        m_gateway->place_none_wait(m_current_order_sell);
    }

    co_return;
}

TaskVoid StrategyMarketMakerStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_order_update");

    // NEW - do nothing
    if (order.status == Order::Status::NEW)
    {
    }
    // FILLED - update order
    else if (order.status == Order::Status::FILLED)
    {
        // 1st order (LIMIT)
        if (order.side == Order::Side::BUY)
        {
            m_current_order_buy = order;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_current_order_sell = order;
        }
    }

    // Check if both orders get FILLED
    if (m_current_order_buy.status == Order::Status::FILLED && m_current_order_sell.status == Order::Status::FILLED)
    {
        m_is_placing = false;
    }

    co_return;
}

TaskVoid StrategyMarketMakerStateRun::update(StrategyUpdateData data)
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