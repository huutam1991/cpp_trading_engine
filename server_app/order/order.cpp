#include <order/order.h>

Order::Order() {}

Order::Order(std::nullptr_t null_value) :
    order_id{0},
    status{Status::NOT_AVAILABLE},
    instrument{nullptr},
    side{Side::BUY},
    type{OrderType::LIMIT},
    price{0.0},
    quantity{0.0}
{}

Order::Order(OrderId order_id_i, Status status_i, const Instrument* instrument_i, Side side_i, const OrderType& type_i, double price_i, double quantity_i) :
    order_id{order_id_i},
    status{status_i},
    instrument{instrument_i},
    side{side_i},
    type{type_i},
    price{price_i},
    quantity{quantity_i}
{}

Json Order::to_json() const
{
    return {
        {"instrument", instrument->to_json()},
        {"error", error.to_json()},
        {"source", {
            {"type", enum_reflect::enum_name(source.type)},
            {"strategy_id", enum_reflect::enum_name(source.strategy_id)}
        }},
        {"created_at", created_at},
        {"order_id", order_id},
        {"status", enum_reflect::enum_name(status)},
        {"side", enum_reflect::enum_name(side)},
        {"type", enum_reflect::enum_name(type)},
        {"price", price},
        {"quantity", quantity},
        {"filled_quantity", filled_quantity},
        {"output_quantity", output_quantity},
        {"volumn_in_quote_currency", volumn_in_quote_currency},
        {"output_asset", commission_asset},
        {"fee", fee},
        {"commission_asset", commission_asset}
    };
}

Order Order::from_json(Json& data)
{
    // Get instrument from Json
    Json instrument_json = data["instrument"];
    ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>((std::string)instrument_json["exchange_id"]);
    std::string exchange_symbol = instrument_json["exchange_symbol"];
    InstrumentType instrument_type = enum_reflect::enum_value<InstrumentType>((std::string)instrument_json["instrument_type"]);

    // Get instrument pointer
    const Instrument* instrument_ptr = Instrument::get_instrument_by_exchange_symbol(exchange_id, instrument_type, exchange_symbol);

    // Get error
    Error error {
        .code = (int)data["error"]["code"],
        .message = data["error"]["message"]
    };

    // Get source
    Source source {
        .type = enum_reflect::enum_value<Source::SourceType>((std::string)data["source"]["type"]),
        .strategy_id = enum_reflect::enum_value<EventBaseID>((std::string)data["source"]["strategy_id"])
    };

    Order res;
    res.instrument = instrument_ptr;
    res.error = error;
    res.source = source;
    res.created_at = (size_t)data["created_at"];
    res.order_id = (OrderId)data["order_id"];
    res.status = enum_reflect::enum_value<Status>(std::string(data["status"]));
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
