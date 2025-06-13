#pragma once

#include <string>

#include <json/json.h>

enum ExchangeEnum
{
    BINANCE,
    COINBASE
};

enum InstrumentType
{
    SPOT,
    PERPETUAL
};

struct Instrument
{
    ExchangeEnum exchange_enum;
    InstrumentType instrument_type;
    std::string symbol;
    std::string exchange_id;
    size_t lot_size;
    double tick_size;

    inline static std::string to_string(ExchangeEnum data)
    {
        switch (data)
        {
            case ExchangeEnum::BINANCE:
                return "binance";
            case ExchangeEnum::COINBASE:
                return "coinbase";
        }

        return "binance";
    }

    inline static std::string to_string(InstrumentType data)
    {
        switch (data)
        {
            case InstrumentType::SPOT:
                return "spot";
            case InstrumentType::PERPETUAL:
                return "perpetual";
        }

        return "spot";
    }

    inline static ExchangeEnum exchange_enum_from_string(std::string data)
    {
        return data == "binance" ? ExchangeEnum::BINANCE : ExchangeEnum::COINBASE;
    }

    inline static InstrumentType instrument_type_from_string(std::string data)
    {
        return data == "spot" ? InstrumentType::SPOT : InstrumentType::PERPETUAL;
    }

    Json to_json()
    {
        return {
            {"exchange_enum", to_string(exchange_enum)},
            {"instrument_type", to_string(instrument_type)},
            {"symbol", symbol},
            {"exchange_id", exchange_id},
            {"lot_size", lot_size},
            {"tick_size", tick_size}
        };
    }

    static Instrument from_json(Json& data)
    {
        return Instrument {
            exchange_enum_from_string((std::string)data["exchange_enum"]), 
            instrument_type_from_string((std::string)data["instrument_type"]),
            (std::string)data["symbol"], 
            (std::string)data["exchange_id"],
            (size_t)data["lot_size"],
            (double)data["tick_size"]
        };
    };
};