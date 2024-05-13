#include <gateways/binance/binance_gateway.h>

BinanceGateway::BinanceGateway(const std::string& key) : m_quoter_spot(key), m_quoter_perpetual(key)
{
}

Json BinanceGateway::place(Order order)
{
    Json response = m_quoter_perpetual.place(order);

    // Get [symbol] + [quantity]
    std::string symbol;
    double quantity = 0;

    // Get fill symbol + quantity
    if (response.has_field("fills"))
    {
        Json fills = response["fills"];

        fills.for_each([&symbol, &quantity](Json& fill)
        {
            ADD_LOG("fill: " << fill);
            symbol = std::string(fill["commissionAsset"]);
            quantity += std::stod(std::string(fill["qty"]));
            quantity -= std::stod(std::string(fill["commission"]));
        });

        ADD_LOG("Spot order place - symbol: " << symbol << ", quantity: " << quantity);
    }

    return {
        {"symbol", symbol + "USDT"},
        {"quantity", quantity}
    };
}