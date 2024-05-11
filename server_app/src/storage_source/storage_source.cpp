#include <utils.h>
#include <storage_source/storage_source.h>
#include <storage_source/binance/storage_source_binance_testnet.h>
#include <storage_source/binance/storage_source_binance_realnet.h>
#include <storage_source/binance/storage_source_binance_simulator.h>
#include <storage_source/coinbase/storage_source_coinbase_testnet.h>
#include <storage_source/coinbase/storage_source_coinbase_realnet.h>
#include <storage_source/binance_futures/storage_source_binance_futures.h>

StorageSource::StorageSource()
{
}

StorageSource::StorageSource(const std::string& user_id) : m_user_id(user_id)
{
}

std::map<std::string, SourceType> StorageSource::m_source_map = {
    {NONE_DB_SOURCE_NAME,               SourceType::NONE},
    {BINANCE_TESTNET_DB_SOURCE_NAME,    SourceType::BINANCE_TESTNET},
    {BINANCE_SPOT_DB_SOURCE_NAME,       SourceType::BINANCE_SPOT},
    {BINANCE_FUTURES_DB_SOURCE_NAME,    SourceType::BINANCE_FUTURES},
    {BINANCE_SIMULATOR_DB_SOURCE_NAME,  SourceType::BINANCE_SIMULATOR},
    {COINBASE_TESTNET_DB_SOURCE_NAME,   SourceType::COINBASE_TESTNET},
    {COINBASE_REALNET_DB_SOURCE_NAME,   SourceType::COINBASE_REALNET},
    {FTX_TESTNET_DB_SOURCE_NAME,        SourceType::FTX_TESTNET},
    {FTX_REALNET_DB_SOURCE_NAME,        SourceType::FTX_REALNET}
};

std::shared_ptr<StorageSource> StorageSource::generate_storage_souce_by_type(SourceType type)
{
    switch (type)
    {
    case SourceType::NONE:
        return std::make_shared<StorageSource>();
        break;
    case SourceType::BINANCE_TESTNET:
        return std::make_shared<StorageSourceBinanceTestnet>();
        break;
    case SourceType::BINANCE_SPOT:
        return std::make_shared<StorageSourceBinanceRealnet>();
        break;
    case SourceType::BINANCE_FUTURES:
        return std::make_shared<StorageSourceBinanceFutures>();
        break;
    case SourceType::BINANCE_SIMULATOR:
        return std::make_shared<StorageSourceBinanceSimulator>();
        break;
    case SourceType::COINBASE_TESTNET:
        return std::make_shared<StorageSourceCoinbaseTestnet>();
        break;
    case SourceType::COINBASE_REALNET:
        return std::make_shared<StorageSourceCoinbaseRealnet>();
        break;
    default:
        break;
    }

    // Return None StorageSource by default
    return std::make_shared<StorageSource>();
}

SourceType StorageSource::get_source_type_by_name(const std::string& source_name)
{
    if (m_source_map.find(source_name) != m_source_map.end())
    {
        return m_source_map[source_name];
    }

    // Return NONE by default
    return SourceType::NONE;
}

bool StorageSource::check_valid_source_name(const std::string& source_name)
{
    return m_source_map.find(source_name) != m_source_map.end();
}

Json StorageSource::get_available_source()
{
    Json res = Json::create_array();
    std::vector<std::string> str_arr;

    for (auto it = m_source_map.begin(); it != m_source_map.end(); it++)
    {
        if (it->first != NONE_DB_SOURCE_NAME)
        {
            str_arr = Utils::instance().split_string(it->first, "_");

            res.push_back(Json {
                {"enviroment", str_arr[1]},
                {"source", str_arr[0]}
            });
        }
    }

    return res;
}

std::string StorageSource::init_info()
{
    return INIT_STORAGE_SOURCE_SUCCESS;
}

std::string StorageSource::verify_valid_source()
{
    return INIT_STORAGE_SOURCE_SUCCESS;
}

const Json StorageSource::get_source_info() const
{
    // If source type is None, return empty json object
    if (get_source_type() == SourceType::NONE)
    {
        return Json();
    }

    std::vector<std::string> str_arr = Utils::instance().split_string(get_db_name(), "_");
    return Json {
        {"enviroment", str_arr[1]},
        {"source", str_arr[0]}
    };
}

const char* StorageSource::get_db_name() const
{
    return NONE_DB_SOURCE_NAME;
}

SourceType StorageSource::get_source_type() const
{
    return SourceType::NONE;
}

void StorageSource::set_user_id(const std::string& user_id)
{
    m_user_id = user_id;
}

const std::string& StorageSource::get_user_id() const
{
    return m_user_id;
}

const std::string StorageSource::get_id() const
{
    return m_user_id + get_db_name();
}