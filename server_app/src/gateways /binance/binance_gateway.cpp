#include <gateways/binance/binance_gateway.h>

BinanceGateway::BinanceGateway(const std::string& key) :
    m_quoter_spot(key),
    m_quoter_perpetual(key),
    m_market_data("BTCUSDT")
{
    m_market_data.set_call_back([this](const std::string& symbol, Json& payload)
    {
        this->on_depth_update(symbol, payload);
    });
    m_market_data.start();
}

void BinanceGateway::on_depth_update(const std::string& symbol, Json& payload)
{
    ADD_LOG("On depth update - symbol: " << symbol << " - depth: " << payload);
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