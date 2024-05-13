
#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>

BinanceQuoterPerpetual::BinanceQuoterPerpetual(const std::string& key) : BinanceQuoter(key)
{
}

std::string& BinanceQuoterPerpetual::get_url()
{
    return m_url;
}

std::string& BinanceQuoterPerpetual::get_port()
{
    return m_port;
}

Json BinanceQuoterPerpetual::get_trade_result_from_response(Json& response)
{
    // Get [symbol] + [quantity]
    std::string symbol;
    double quantity = 0;

    // Get fill symbol + quantity
    if (response.has_field("symbol") && response.has_field("origQty"))
    {
        symbol = std::string(response["symbol"]);
        quantity = std::stod(std::string(response["origQty"]));

        ADD_LOG("Perpetual order place - symbol: " << symbol << ", quantity: " << quantity);
    }

    return {
        {"type", "perpetual"},
        {"symbol", symbol + "USDT"},
        {"quantity", quantity}
    };
}

Json BinanceQuoterPerpetual::place(Order order)
{
    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;
    std::string side = order.side == Order::Side::BUY ? "BUY" : "SELL";

    query_str += "symbol=" + order.symbol;
    query_str += "&side=" + side;
    query_str += "&type=" + order.type;
    query_str += "&quantity=" + std::to_string(order.quantity);

    if (order.type == "LIMIT")
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    return send_binance_request(RequestMethod::POST, "/fapi/v1/order", query_str);
}