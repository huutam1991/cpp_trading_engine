#include <instrument/instrument.h>

Json Instrument::to_json()
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

Instrument Instrument::from_json(Json& data)
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