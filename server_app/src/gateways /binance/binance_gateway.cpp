#include <gateways/binance/binance_gateway.h>

BinanceGateway::BinanceGateway(const std::string& key) : m_quoter_spot(key), m_quoter_perpetual(key)
{
}

Json BinanceGateway::place(Order order)
{
    Json response = m_quoter_perpetual.place(order);

    return m_quoter_perpetual.get_trade_result_from_response(response);
}