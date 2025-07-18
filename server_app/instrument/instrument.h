#pragma once

#include <string>
#include <mutex>

#include <json/json.h>
#include <symbol/symbol.h>
#include <utils/spin_lock.h>
#include <enum_reflect/enum_reflect.h>
#include <data_model/savable_object.h>
#include <app_constants.h>

enum ExchangeId
{
    BINANCE,
    COINBASE,
    TOTAL
};

enum InstrumentType
{
    SPOT,
    PERPETUAL,
    TOTAL
};

class Instrument
{
public:
    ExchangeId exchange_id;
    InstrumentType instrument_type;
    Symbol symbol;
    Symbol exchange_symbol;
    size_t lot_size;
    double tick_size;

    Json to_json() const;
    static Instrument from_json(Json& data);

    // Helper method
    std::string round_string_number(const std::string& str_number, size_t precision);
    double get_round_up_quantity(double quantity);

    using CacheInstruments = std::unordered_map<std::string, SavableObject<Instrument>>;
    
    static CacheInstruments& load_cache_instruments(ExchangeId exchange_id);
    static void add_instrument_to_cache(ExchangeId exchange_id, const Instrument& instrument);
    static const Instrument* get_instrument_by_symbol(ExchangeId exchange_id, InstrumentType instrument_type, const std::string& symbol);
    static const Instrument* get_instrument_by_exchange_symbol(ExchangeId exchange_id, InstrumentType instrument_type, const std::string& symbol);

private:
    enum StoreType
    {
        BY_SYMBOL,
        BY_EXCHANGE_SYMBOL,
        TOTAL
    };

    static CacheInstruments& get_cache_instruments(ExchangeId exchange_id);
    static void clear_instrument_by_exchange(ExchangeId exchange_id);
    static void add_instrument_to_list(ExchangeId exchange_id, const Instrument& instrument);

    static std::unordered_map<std::string, const Instrument*>& get_instrument_list(ExchangeId exchange_id, InstrumentType instrument_type, StoreType store_type)
    {
        using InstrumentList = std::unordered_map<std::string, const Instrument*>;
        static InstrumentList instrument_list[ExchangeId::TOTAL][InstrumentType::TOTAL][StoreType::TOTAL];

        return instrument_list[exchange_id][instrument_type][store_type];
    }
};