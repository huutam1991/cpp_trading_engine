#include <gateways/binance/binance_gateway.h>

BinanceGateway::BinanceGateway(const std::string& key) :
    m_quoter_spot(key),
    m_quoter_perpetual(key),
    m_market_data_sport(BINANCE_SPOT_WS_URL, BINANCE_SPOT_WS_PORT),
    m_market_data_perpetual(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT)
{
}

void BinanceGateway::subscribe_symbol(const std::string& symbol)
{
    // Spot
    m_market_data_sport.subscribe_symbol(symbol, [this](const std::string& symbol, Json& payload)
    {
        this->on_depth_update(symbol, payload);
    });
    m_market_data_sport.start();

    // Perpetual
    // m_market_data_perpetual.subscribe_symbol(symbol, [this](const std::string& symbol, Json& payload)
    // {
    //     this->on_depth_update(symbol, payload);
    // });
    // m_market_data_perpetual.start();
}

void BinanceGateway::on_depth_update(const std::string& symbol, Json& payload)
{
    double best_bid = payload["bids"][0][0];
    double best_ask = payload["asks"][0][0];
    ADD_LOG("On depth update - symbol: " << symbol << " - best_bid: " << best_bid << " - best_ask: " << best_ask);

    m_price_update_callback(best_ask);
}

Json BinanceGateway::place(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.exchange_type == Order::ExchangeType::SPOT ?
        (BinanceQuoter*)&m_quoter_spot :
        (BinanceQuoter*)&m_quoter_perpetual;

    Json response = quoter->place(order);
    // Tricky here, assume value of price from [response] is current market price
    response["price"] = order.price;

    return quoter->get_trade_result_from_response(response);
}