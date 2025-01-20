#ifndef ORDER_H
#define ORDER_H

#include <string>

class Order
{
public:
    enum ExchangeType
    {
        SPOT,
        PERPETUAL
    };

    enum Side
    {
        BUY,
        SELL
    };

    enum OrderType
    {
        LIMIT,
        MARKET
    };

    ExchangeType exchange_type;
    std::string symbol;
    Side side;
    OrderType type;
    double price;
    double quantity;

    Order() {}
    Order(ExchangeType exchange_type_i, const std::string& symbol_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i) :
        exchange_type{exchange_type_i},
        symbol{symbol_i},
        side{side_i},
        type{type_i},
        price{price_i},
        quantity{quantity_i}
    {}

    inline static std::string to_string(Side side)
    {
        return side == Side::SELL ? "SELL" : "BUY";
    }

    inline static std::string to_string(OrderType type)
    {
        return type == OrderType::LIMIT ? "LIMIT" : "MARKET";
    }

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"exchange_type", exchange_type == ExchangeType::SPOT ? "spot" : "perpetual"},
            {"side", Order::to_string(side)},
            {"type", Order::to_string(type)},
            {"price", price},
            {"quantity", quantity},
        };
    }
};

#endif //ORDER_H