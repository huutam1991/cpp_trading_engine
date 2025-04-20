#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <json/json.h>

using OrderId = size_t;

class Order
{
public:
    enum ExchangeType
    {
        SPOT,
        PERPETUAL
    };

    enum Status
    {
        NOT_AVAILABLE,
        NEW,
        CANCELED,
        REJECTED,
        PARTIALLY_FILLED,
        FILLED,
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

    // Input data
    OrderId order_id;
    ExchangeType exchange_type;
    Status status = Status::NOT_AVAILABLE;
    std::string symbol;
    Side side;
    OrderType type;
    double price;
    double quantity;

    // Output data
    double filled_quantity = 0.0; // Always for base currency
    double filled_price = 0.0;
    double commission_amount = 0.0; // Can be either base currency or quote currency
    double output_quantity = 0.0; // Can be either base currency or quote currency
    double volumn_in_quote_currency = 0.0; // Volumn of the order in quote currency
    std::string commission_asset = "";
    std::string output_asset = ""; // The same with commission_asset

    Order();
    Order(OrderId order_id_i, ExchangeType exchange_type_i, Status status_i, const std::string& symbol_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i);

    inline static std::string to_string(ExchangeType data)
    {
        return data == ExchangeType::SPOT ? "SPOT" : "PERPETUAL";
    }

    inline static std::string to_string(Status data)
    {
        switch (data)
        {
        case Status::NOT_AVAILABLE:
            return "NOT_AVAILABLE";
        case Status::NEW:
            return "NEW";
        case Status::CANCELED:
            return "CANCELED";
        case Status::REJECTED:
            return "REJECTED";
        case Status::FILLED:
            return "FILLED";

        default:
            return "NEW";
        }
    }

    inline static std::string to_string(Side data)
    {
        return data == Side::SELL ? "SELL" : "BUY";
    }

    inline static std::string to_string(OrderType data)
    {
        return data == OrderType::LIMIT ? "LIMIT" : "MARKET";
    }

    inline static ExchangeType exchange_type_from_string(std::string data)
    {
        return data == "SPOT" ? ExchangeType::SPOT : ExchangeType::PERPETUAL;
    }

    inline static Status status_from_string(std::string data)
    {
        Status res;
        if (data == "NOT_AVAILABLE")
        {
            res = Status::NOT_AVAILABLE;
        }
        else if (data == "NEW")
        {
            res = Status::NEW;
        }
        else if (data == "CANCELED")
        {
            res = Status::CANCELED;
        }
        else if (data == "REJECTED")
        {
            res = Status::REJECTED;
        }
        else if (data == "PARTIALLY_FILLED")
        {
            res = Status::PARTIALLY_FILLED;
        }
        else if (data == "FILLED")
        {
            res = Status::FILLED;
        }

        return res;
    }

    inline static Side side_from_string(std::string data)
    {
        return data == "BUY" ? Side::BUY : Side::SELL;
    }

    inline static OrderType type_from_string(std::string data)
    {
        return data == "LIMIT" ? OrderType::LIMIT : OrderType::MARKET;
    }

    Json to_json();
    static Order from_json(Json& data);
};

#endif //ORDER_H