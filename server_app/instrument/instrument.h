#pragma once

#include <string>

#include <json/json.h>
#include <symbol/symbol.h>

enum ExchangeId
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
    ExchangeId exchange_id;
    InstrumentType instrument_type;
    Symbol symbol;
    Symbol exchange_symbol;
    size_t lot_size;
    double tick_size;

    inline static std::string to_string(ExchangeId data)
    {
        switch (data)
        {
            case ExchangeId::BINANCE:
                return "binance";
            case ExchangeId::COINBASE:
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

    inline static ExchangeId exchange_id_from_string(std::string data)
    {
        return data == "binance" ? ExchangeId::BINANCE : ExchangeId::COINBASE;
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