#pragma once

#include <string>
#include <json/json.h>
#include <enum_reflect/enum_reflect.h>
#include <symbol/symbol.h>
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
        MARKET
    };

    // Input data
    OrderId order_id;
    InstrumentType instrument_type;
    Status status = Status::NOT_AVAILABLE;
    Symbol symbol;
    Symbol exchange_symbol;
    Side side;
    OrderType type;
    double price = 0.0;
    double quantity = 0.0;

    // Output data
    double filled_quantity = 0.0; // Always for base currency
    double filled_price = 0.0;
    double commission_amount = 0.0; // Can be either base currency or quote currency
    double output_quantity = 0.0; // Can be either base currency or quote currency
    double volumn_in_quote_currency = 0.0; // Volumn of the order in quote currency
    Symbol commission_asset;
    Symbol output_asset; 

    Order();
    Order(OrderId order_id_i, InstrumentType exchange_type_i, Status status_i, const Symbol& symbol_i, const Symbol& exchange_symbol_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i);

    Json to_json();
    static Order from_json(Json& data);
};
