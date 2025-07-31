#pragma once

#include <string>
#include <c_json/json.h>
#include <enum_reflect/enum_reflect.h>
#include <order/order.h>

struct BuyPoint
{
    enum Status
    {
        AVAILABLE,
        PLACING,
        PLACED,
        CANCELING,
        HOLD,
    };

    double price = 0.0;
    double quantity = 0.0;
    double input_in_usdt = 0.0;
    double output_in_usdt = 0.0;
    double profit = 0.0;
    OrderId current_order_id = 0;
    Status status = Status::AVAILABLE;

    JsonNew to_json()
    {
        return {
            {"price", price},
            {"quantity", quantity},
            {"input_in_usdt", input_in_usdt},
            {"output_in_usdt", output_in_usdt},
            {"profit", profit},
            {"current_order_id", current_order_id},
            {"status", enum_reflect::enum_name(status)},
        };
    }

    static BuyPoint from_json(JsonNew& data)
    {
        BuyPoint res;

        // Only load from [data], if it is valid
        if (data.has_field("status"))
        {
            res.price = (double)data["price"];
            res.quantity = (double)data["quantity"];
            res.input_in_usdt = (double)data["input_in_usdt"];
            res.output_in_usdt = (double)data["output_in_usdt"];
            res.profit = (double)data["profit"];
            res.current_order_id = (OrderId)data["current_order_id"];
            res.status = enum_reflect::enum_value<Status>((std::string)data["status"]);
        }

        return res;
    }
};
