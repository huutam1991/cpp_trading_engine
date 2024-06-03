
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>

BinanceQuoterSpot::BinanceQuoterSpot(const std::string& key) : BinanceQuoter(key)
{
    m_url = m_is_testnet == true ? BINANCE_TESTNET_SPOT_URL : BINANCE_SPOT_URL;
    m_port = m_is_testnet == true ? BINANCE_TESTNET_SPOT_PORT : BINANCE_SPOT_PORT;
}

std::string& BinanceQuoterSpot::get_url()
{
    return m_url;
}

std::string& BinanceQuoterSpot::get_port()
{
    return m_port;
}

Json BinanceQuoterSpot::get_trade_result_from_response(Json& response)
{
    // Get [symbol] + [quantity]
    std::string symbol;
    double quantity = 0;
    double volumn_in_usdt = 0;

    // Get fill symbol + quantity
    if (response.has_field("fills"))
    {
        Json fills = response["fills"];

        fills.for_each([&](Json& fill)
        {
            ADD_LOG("fill: " << fill);
            double f_quantity = std::stod(std::string(fill["qty"]));
            double f_commission = std::stod(std::string(fill["commission"]));
            double f_price = std::stod(std::string(fill["price"]));
            symbol = std::string(fill["commissionAsset"]);

            // This is BUY order
            if (symbol != "USDT")
            {
                quantity += f_quantity - f_commission;
                volumn_in_usdt += f_quantity * f_price;
            }
            // This is SELL order
            else
            {
                quantity += (f_quantity * f_price) - f_commission;
            }
        });

        ADD_LOG("Spot order place - symbol: " << symbol << ", quantity: " << quantity);
    }

    return {
        {"type", "spot"},
        {"symbol", symbol},
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt}
    };
}

Json BinanceQuoterSpot::place(Order order)
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

    return send_binance_request(RequestMethod::POST, "/api/v3/order", query_str);
}
