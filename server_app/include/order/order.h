#ifndef ORDER_H
#define ORDER_H

#include <string>

class Order
{
public:
    enum Side
    {
        BUY,
        SELL
    };

    Side side;
    std::string type;
    double price;
    double quantity;

    Order(Side side_i, std::string type_i, double price_i, double quantity_i) :
        side{side_i},
        type{type_i},
        price{price_i},
        quantity{quantity_i}
    {}


};

#endif //ORDER_H