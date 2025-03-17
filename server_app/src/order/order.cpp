#include <order/order.h>

Order::Order() {}

Order::Order(OrderId order_id_i, ExchangeType exchange_type_i, Status status_i, const std::string& symbol_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i) :
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
        {"symbol", symbol},
        {"side", Order::to_string(side)},
        {"type", Order::to_string(type)},
        {"price", price},
        {"filled_quantity", filled_quantity},
        {"output_quantity", output_quantity},
        {"output_asset", commission_asset},
        {"commission_amount", commission_amount},
        {"commission_asset", commission_asset},
    };
}
