#include <order/order.h>

Order::Order() {}

Order::Order(OrderId order_id_i, InstrumentType exchange_type_i, Status status_i, const std::string& symbol_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i) :
    order_id{order_id_i},
    exchange_type{exchange_type_i},
    status{status_i},
    symbol{symbol_i},
    side{side_i},
    type{type_i},
    price{price_i},
    quantity{quantity_i}
{}

Json Order::to_json()
{
    return {
        {"order_id", order_id},
        {"exchange_type", Order::to_string(exchange_type)},
        {"status", Order::to_string(status)},
        {"symbol", symbol.to_string()},
        {"side", Order::to_string(side)},
        {"type", Order::to_string(type)},
        {"price", price},
        {"quantity", quantity},
        {"filled_quantity", filled_quantity},
        {"output_quantity", output_quantity},
        {"volumn_in_quote_currency", volumn_in_quote_currency},
        {"output_asset", commission_asset.to_string()},
        {"commission_amount", commission_amount},
        {"commission_asset", commission_asset.to_string()},
    };
}


Order Order::from_json(Json& data)
{
    Order res;

    res.order_id = (OrderId)data["order_id"];
    res.exchange_type = Order::exchange_type_from_string(std::string(data["exchange_type"]));
    res.status = Order::status_from_string(std::string(data["status"]));
    res.symbol = (std::string)data["symbol"];
    res.side = Order::side_from_string(std::string(data["side"]));
    res.type = Order::type_from_string(std::string(data["type"]));
    res.price = (double)data["price"];
    res.quantity = (double)data["quantity"];
    res.filled_quantity = (double)data["filled_quantity"];
    res.filled_price = 0.0;
    res.commission_amount = (double)data["commission_amount"];
    res.output_quantity = (double)data["output_quantity"];
    res.volumn_in_quote_currency = (double)data["volumn_in_quote_currency"];
    res.commission_asset = (std::string)data["commission_asset"];
    res.output_asset = (std::string)data["output_asset"];

    return res;
}
