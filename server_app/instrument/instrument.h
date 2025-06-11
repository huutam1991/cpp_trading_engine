#pragma once

#include <string>

#include <json/json.h>

struct Instrument
{
    std::string symbol;
    std::string exchange_id;
    size_t lot_size;
    double tick_size;

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"exchange_id", exchange_id},
            {"lot_size", lot_size},
            {"tick_size", tick_size}
        };
    }

    static Instrument from_json(Json& data)
    {
        return Instrument {
            (std::string)data["symbol"], 
            (std::string)data["exchange_id"],
            (size_t)data["lot_size"],
            (double)data["tick_size"]
        };
    };
};