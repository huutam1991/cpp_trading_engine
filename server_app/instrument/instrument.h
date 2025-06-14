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
    ExchangeEnum exchange_id;
    InstrumentType instrument_type;
    std::string symbol;
    std::string exchange_symbol;
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

    Json to_json();
    static Instrument from_json(Json& data);

    // Helper method
    std::string round_string_number(const std::string& str_number, size_t precision);
    double get_round_up_quantity(double quantity);
};