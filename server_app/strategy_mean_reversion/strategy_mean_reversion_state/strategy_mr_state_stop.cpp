#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>
#include <time/measure_time.h>

StrategyMeanReversionStateStop::StrategyMeanReversionStateStop(
    std::shared_ptr<Gateway> gateway,
    const StrategyMeanReversionConfig& config,
    SpreadCaptureConfigManager& spread_captures)
    : m_gateway{gateway}, m_config{config}, m_spread_captures{spread_captures}
{
}

void StrategyMeanReversionStateStop::begin()
{
    spdlog::info("StrategyMeanReversionStateStop - begin");
}

void StrategyMeanReversionStateStop::end()
{
    spdlog::info("StrategyMeanReversionStateStop - end");
}

Json StrategyMeanReversionStateStop::get_info()
{
    return {
        {"spread_captures", m_spread_captures.get_info()},
        {"current_price", m_current_price}
    };
}

void StrategyMeanReversionStateStop::handle_price_update(PriceUpdate& price)
{
}

void StrategyMeanReversionStateStop::handle_trade_update(TradeUpdate& trade)
{
}

void StrategyMeanReversionStateStop::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    PipelineTraceBuffer::RecordStageTiming<"strategy_mean_reversion"> record_stage(snapshot->trace_id);

    // MeasureTime t("StrategyMeanReversionStateStop - handle_order_book_snapshot");
    m_current_price = snapshot->get_mid_price();
    m_spread_captures.handle_order_book_snapshot(snapshot);

    ScopeTiming pipeline_timing = PipelineTraceBuffer::get_pipeline_timing<"market_data_received", "strategy_mean_reversion">(snapshot->trace_id);
    spdlog::debug("symbol: {}, pipeline timing: {} ticks, {} ns, {} us",
        snapshot->instrument->symbol, pipeline_timing.ticks, pipeline_timing.ns, pipeline_timing.us);
}

void StrategyMeanReversionStateStop::handle_order_update(Order& order)
{
    // spdlog::info("StrategyMeanReversionStateStop: do nothing, symbol: {}, side: {}, price: {}, quantity: {}, status: {}",
    //     order.instrument->symbol, enum_reflect::enum_name(order.side), order.price, order.quantity, enum_reflect::enum_name(order.status));
}