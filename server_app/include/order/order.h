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

    ExchangeType exchange_type;
    std::string symbol;
    Side side;
    std::string type;
    double price;
    double quantity;

    Order() {}
    Order(ExchangeType exchange_type_i, const std::string& symbol_i, Side side_i, const std::string& type_i, double price_i, double quantity_i) :
        exchange_type{exchange_type_i},
        symbol{symbol_i},
        side{side_i},
        type{type_i},
        price{price_i},
        quantity{quantity_i}
    {}

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"exchange_type", exchange_type == ExchangeType::SPOT ? "spot" : "perpetual"},
            {"side", side == Side::BUY ? "buy" : "sell"},
            {"type", type},
            {"price", price},
            {"quantity", quantity},
        };
    }

};

#endif //ORDER_H