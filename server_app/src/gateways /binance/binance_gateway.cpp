#include <gateways/binance/binance_gateway.h>

BinanceGateway::BinanceGateway(const std::string& key) : m_quoter_spot(key), m_quoter_perpetual(key)
{
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