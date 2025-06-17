#pragma once

#include <string>
#include <json/json.h>

struct BuyPoint
{
    enum Status
    {
        AVAILABLE,
        PLACING,
        PLACED,
        HOLD,
    };

    double price = 0.0;
    double quantity = 0.0;
    size_t profit = 0.0;
    Status status = Status::AVAILABLE;

    static std::string to_string(Status data)
    {
        switch (data)
        {
        case Status::AVAILABLE:
            return "available";
        case Status::PLACING:
            return "placing";
        case Status::PLACED:
            return "placed";
        case Status::HOLD:
            return "hold";
        }

        return "available";
    }

    static Status from_string(const std::string& data)
    {
        if (data == "available")
        {
            return Status::AVAILABLE;
        }
        else if (data == "placing")
        {
            return Status::PLACING;
        }
        else if (data == "placed")
        {
            return Status::PLACED;
        }
        else if (data == "hold")
        {
            return Status::HOLD;
        }

        return Status::AVAILABLE;
    }

    Json to_json()
    {
        return {
            {"price", price},
            {"quantity", quantity},
            {"profit", profit},
            {"status", to_string(status)},
        };
    }

    static BuyPoint from_json(Json& data)
    {
        BuyPoint res;

        // Only load from [data], if it is valid
        if (data.has_field("status"))
        {
            res.price = (double)data["price"];
            res.quantity = (double)data["quantity"];
            res.profit = (double)data["profit"];
            res.status = from_string((std::string)data["status"]);
        }

        return res;
    }
};
