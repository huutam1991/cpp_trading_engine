#pragma once

#include <string>

#include <json/json.h>
#include <symbol/symbol.h>
#include <enum_reflect/enum_reflect.h>
#include <data_model/savable_object.h>

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

    Json to_json() const;
    static Instrument from_json(Json& data);

    // Helper method
    std::string round_string_number(const std::string& str_number, size_t precision);
    double get_round_up_quantity(double quantity);

    enum StoreType
    {
        BY_SYMBOL,
        BY_EXCHANGE_SYMBOL,
        TOTAL
    };
    
    static std::unordered_map<ExchangeId, std::unordered_map<std::string, Instrument>>& get_instrument_list(StoreType store_type)
    {
        using InstrumentStoreType = std::unordered_map<ExchangeId, std::unordered_map<std::string, Instrument>>;
        static InstrumentStoreType instrument_list[StoreType::TOTAL];

        return instrument_list[store_type];
    }
    
    static std::unordered_map<std::string, Instrument>& get_instrument_list_by_exchange(ExchangeId exchange_id, StoreType store_type)
    {
        auto& instrument_list = get_instrument_list(store_type);

        if (instrument_list.find(exchange_id) == instrument_list.end())
        {
            instrument_list.insert(std::make_pair(exchange_id, std::unordered_map<std::string, Instrument>()));
        }

        return instrument_list.find(exchange_id)->second;
    }

    static void create_instrument_list_by_exchange_symbol(ExchangeId exchange_id)
    {
        std::unordered_map<std::string, Instrument>& ins_by_symbol = get_instrument_list_by_exchange(exchange_id, StoreType::BY_SYMBOL);
        std::unordered_map<std::string, Instrument>& ins_by_exchange_symbol = get_instrument_list_by_exchange(exchange_id, StoreType::BY_EXCHANGE_SYMBOL);

        for (auto& [_, instrument] : ins_by_symbol)
        {
            ins_by_exchange_symbol.insert(std::make_pair(instrument.exchange_symbol, instrument));
        }
    }

    static Instrument* get_instrument_by_symbol(ExchangeId exchange_id, const std::string& symbol)
    {
        std::unordered_map<std::string, Instrument>& instruments = get_instrument_list_by_exchange(exchange_id, StoreType::BY_SYMBOL);
        auto it = instruments.find(symbol);
        if (it != instruments.end())
        {
            return &it->second;
        }
        else
        {
            throw std::runtime_error("Cannot find instrument with symbol: [" + symbol + "]");
        }

        return nullptr;
    }

    static Instrument* get_instrument_by_exchange_symbol(ExchangeId exchange_id, const std::string& symbol)
    {
        std::unordered_map<std::string, Instrument>& instruments = get_instrument_list_by_exchange(exchange_id, StoreType::BY_EXCHANGE_SYMBOL);
        auto it = instruments.find(symbol);
        if (it != instruments.end())
        {
            return &it->second;
        }
        else
        {
            throw std::runtime_error("Cannot find instrument with exchange symbol: [" + symbol + "]");
        }

        return nullptr;
    }
};