#include <order/order.h>

Order::Order() {}

Order::Order(OrderId order_id_i, InstrumentType exchange_type_i, Status status_i, const Instrument* instrument_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i) :
    order_id{order_id_i},
    instrument_type{exchange_type_i},
    status{status_i},
    instrument{instrument_i},
    side{side_i},
    type{type_i},
    price{price_i},
    quantity{quantity_i}
{}

Json Order::to_json()
{
    return {
        {"order_id", order_id},
        {"instrument_type", enum_reflect::enum_name(instrument_type)},
        {"status", enum_reflect::enum_name(status)},
        {"instrument", instrument->to_json()},
        {"side", enum_reflect::enum_name(side)},
        {"type", enum_reflect::enum_name(type)},
        {"price", price},
        {"quantity", quantity},
        {"filled_quantity", filled_quantity},
        {"output_quantity", output_quantity},
        {"volumn_in_quote_currency", volumn_in_quote_currency},
        {"output_asset", commission_asset},
        {"fee", fee},
        {"commission_asset", commission_asset},
    };
}


Order Order::from_json(Json& data)
{
    // Get instrument from Json
    Json instrument_json = data["instrument"];
    ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>(std::string(instrument_json["exchange_id"]));
    std::string symbol = instrument_json["symbol"];
    InstrumentType instrument_type = enum_reflect::enum_value<InstrumentType>(std::string(data["instrument_type"]));

    // Get instrument pointer
    const Instrument* instrument_ptr = Instrument::get_instrument_by_symbol(exchange_id, instrument_type, symbol);

    Order res;
    res.order_id = (OrderId)data["order_id"];
    res.instrument_type = instrument_type;
    res.status = enum_reflect::enum_value<Status>(std::string(data["status"]));
    res.instrument = instrument_ptr;
    res.side = enum_reflect::enum_value<Side>(std::string(data["side"]));
    res.type = enum_reflect::enum_value<OrderType>(std::string(data["type"]));
    res.price = (double)data["price"];
    res.quantity = (double)data["quantity"];
    res.filled_quantity = (double)data["filled_quantity"];
    res.filled_price = 0.0;
    res.fee = (double)data["fee"];
    res.output_quantity = (double)data["output_quantity"];
    res.volumn_in_quote_currency = (double)data["volumn_in_quote_currency"];
    res.commission_asset = data["commission_asset"];
    res.output_asset = data["output_asset"];

    return res;
}
