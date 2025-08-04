#include <instrument/instrument.h>

Json Instrument::to_json() const
{
    return {
        {"exchange_id", enum_reflect::enum_name(exchange_id)},
        {"instrument_type", enum_reflect::enum_name(instrument_type)},
        {"symbol", symbol},
        {"exchange_symbol", exchange_symbol},
        {"lot_size", lot_size},
        {"tick_size", tick_size}
    };
}

Instrument Instrument::from_json(Json& data)
{
    return Instrument {
        enum_reflect::enum_value<ExchangeId>((std::string)data["exchange_id"]),
        enum_reflect::enum_value<InstrumentType>((std::string)data["instrument_type"]),
        data["symbol"],
        data["exchange_symbol"],
        (size_t)data["lot_size"],
        (double)data["tick_size"]
    };
};

std::string Instrument::round_string_number(const std::string& str_number, size_t precision) const
{
    int point_pos = str_number.find_first_of(".");
    if (point_pos > -1)
    {
        return str_number.substr(0, point_pos + (precision == 0 ? 0 : precision + 1));
    }

    return str_number;
}

double Instrument::get_round_up_quantity(double quantity) const
{
    std::string round_str_number = round_string_number(std::to_string(quantity), lot_size);
    return std::stod(round_str_number);
}

Instrument::CacheInstruments& Instrument::get_cache_instruments(ExchangeId exchange_id)
{
    static std::unordered_map<ExchangeId, CacheInstruments> cache_instruments_by_exchange;
    static std::mutex cache_instruments_mutex;

    std::unique_lock<std::mutex> lock(cache_instruments_mutex);

    // Check if already loaded
    if (cache_instruments_by_exchange.find(exchange_id) == cache_instruments_by_exchange.end())
    {
        std::string exchange_name(enum_reflect::enum_name(exchange_id));
        CacheInstruments cache_instruments = SavableObject<Instrument>::load_objects_map<std::string>(INSTRUMENT_DB_NAME, exchange_name, "symbol");
        cache_instruments_by_exchange.emplace(exchange_id, std::move(cache_instruments));
    }

    return cache_instruments_by_exchange[exchange_id];
}

Instrument::CacheInstruments& Instrument::load_cache_instruments(ExchangeId exchange_id)
{
    CacheInstruments& cache_instruments = get_cache_instruments(exchange_id);

    // Clear existing instruments for this exchange
    clear_instrument_by_exchange(exchange_id);

    // Add instruments to the static list
    for (auto& [_, instrument] : cache_instruments)
    {
        add_instrument_to_list(exchange_id, instrument.object);
    }

    return cache_instruments;
}

void Instrument::add_instrument_to_cache(ExchangeId exchange_id, const Instrument& instrument)
{
    CacheInstruments& cache_instruments = get_cache_instruments(exchange_id);
    std::string exchange_name(enum_reflect::enum_name(exchange_id));
    std::string symbol = instrument.symbol.to_string();

    // Add to cache
    auto [it, success] = cache_instruments.emplace(symbol, SavableObject<Instrument>(INSTRUMENT_DB_NAME, exchange_name));
    if (success == true)
    {
        it->second = instrument; // Update to DB
    }
    else
    {
        throw std::runtime_error("Cannot add instrument to cache: [" + symbol + "]");
    }

    // Add to static list
    add_instrument_to_list(exchange_id, it->second.object);
}

void Instrument::clear_instrument_by_exchange(ExchangeId exchange_id)
{
    for (size_t i = 0; i < InstrumentType::TOTAL_INSTRUMENTS; i++)
    {
        for (size_t j = 0; j < StoreType::TOTAL_STORE_TYPES; j++)
        {
            get_instrument_list(exchange_id, (InstrumentType)i, (StoreType)j).clear();
        }
    }
}

void Instrument::add_instrument_to_list(ExchangeId exchange_id, const Instrument& instrument)
{
    std::unordered_map<std::string, const Instrument*>& ins_by_symbol = get_instrument_list(exchange_id, instrument.instrument_type, StoreType::BY_SYMBOL);
    std::unordered_map<std::string, const Instrument*>& ins_by_exchange_symbol = get_instrument_list(exchange_id, instrument.instrument_type, StoreType::BY_EXCHANGE_SYMBOL);

    ins_by_symbol.emplace(instrument.symbol, &instrument);
    ins_by_exchange_symbol.emplace(instrument.exchange_symbol, &instrument);
}

const Instrument* Instrument::get_instrument_by_symbol(ExchangeId exchange_id, InstrumentType instrument_type, const std::string& symbol)
{
    std::unordered_map<std::string, const Instrument*>& instruments = get_instrument_list(exchange_id, instrument_type, StoreType::BY_SYMBOL);
    auto it = instruments.find(symbol);
    if (it != instruments.end())
    {
        return it->second;
    }
    else
    {
        throw std::runtime_error("Cannot find instrument with symbol: [" + symbol + "]");
    }

    return nullptr;
}

const Instrument* Instrument::get_instrument_by_exchange_symbol(ExchangeId exchange_id, InstrumentType instrument_type, const std::string& symbol)
{
    std::unordered_map<std::string, const Instrument*>& instruments = get_instrument_list(exchange_id, instrument_type, StoreType::BY_EXCHANGE_SYMBOL);
    auto it = instruments.find(symbol);
    if (it != instruments.end())
    {
        return it->second;
    }
    else
    {
        throw std::runtime_error("Cannot find instrument with exchange symbol: [" + symbol + "]");
    }

    return nullptr;
}