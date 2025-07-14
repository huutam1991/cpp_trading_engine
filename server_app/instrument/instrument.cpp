#include <instrument/instrument.h>

Json Instrument::to_json() const
{
    return {
        {"exchange_id", enum_reflect::enum_name(exchange_id)},
        {"instrument_type", enum_reflect::enum_name(instrument_type)},
        {"symbol", (std::string)symbol},
        {"exchange_symbol", (std::string)exchange_symbol},
        {"lot_size", lot_size},
        {"tick_size", tick_size}
    };
}

Instrument Instrument::from_json(Json& data)
{
    return Instrument {
        enum_reflect::enum_value<ExchangeId>((std::string)data["exchange_id"]), 
        enum_reflect::enum_value<InstrumentType>((std::string)data["instrument_type"]),
        Symbol((std::string)data["symbol"]), 
        Symbol((std::string)data["exchange_symbol"]),
        (size_t)data["lot_size"],
        (double)data["tick_size"]
    };
};

std::string Instrument::round_string_number(const std::string& str_number, size_t precision)
{
    int point_pos = str_number.find_first_of(".");
    if (point_pos > -1)
    {
        return str_number.substr(0, point_pos + (precision == 0 ? 0 : precision + 1));
    }

    return str_number;
}

double Instrument::get_round_up_quantity(double quantity)
{
    std::string round_str_number = round_string_number(std::to_string(quantity), lot_size);
    return std::stod(round_str_number);
}