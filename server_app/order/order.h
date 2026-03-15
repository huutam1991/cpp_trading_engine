#pragma once

#include <string>
#include <json/json.h>
#include <enum_reflect/enum_reflect.h>
#include <instrument/instrument.h>

using OrderId = size_t;

class Order
{
public:

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
        MARKET,
        TAKE_PROFIT_MARKET,
        STOP_LOSS_MARKET,
        TAKE_PROFIT_LIMIT,
        STOP_LOSS_LIMIT,
        TRAILING_STOP_MARKET,
        POST_ONLY,
        TOTAL_ORDER_TYPES
    };

    // Input data
    OrderId order_id = 0;
    Status status = Status::NOT_AVAILABLE;
    const Instrument *instrument;
    Side side;
    OrderType type;
    double price = 0.0;
    double quantity = 0.0;

    // Output data
    double filled_quantity = 0.0; // Always for base currency
    double filled_price = 0.0;
    double fee = 0.0; // Can be either base currency or quote currency
    double output_quantity = 0.0; // Can be either base currency or quote currency
    double volumn_in_quote_currency = 0.0; // Volumn of the order in quote currency
    ShareString commission_asset;
    ShareString output_asset;

    bool operator==(std::nullptr_t) const
    {
        return order_id == 0 && status == Status::NOT_AVAILABLE;
    }

    Order& operator=(std::nullptr_t)
    {
        order_id = 0;
        status = Status::NOT_AVAILABLE;
        instrument = nullptr;
        side = Side::BUY;
        type = OrderType::LIMIT;
        price = 0.0;
        quantity = 0.0;

        return *this;
    }

    Order();
    Order(std::nullptr_t null_value);
    Order(OrderId order_id_i, Status status_i, const Instrument* instrument_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i);

    Json to_json() const;
    static Order from_json(Json& data);
};
