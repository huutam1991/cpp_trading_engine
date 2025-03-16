#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <json/json.h>

using OrderId = uint64_t;

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

    OrderId order_id;
    ExchangeType exchange_type;
    std::string symbol;
    Side side;
    OrderType type;
    double price;
    double quantity;

    Order();
    Order(OrderId order_id_i, ExchangeType exchange_type_i, const std::string& symbol_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i);

    inline static std::string to_string(Side side)
    {
        return side == Side::SELL ? "SELL" : "BUY";
    }

    inline static std::string to_string(OrderType type)
    {
        return type == OrderType::LIMIT ? "LIMIT" : "MARKET";
    }

    Json to_json();
};

#endif //ORDER_H