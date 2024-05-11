#include <binance_utils.h>
#include <utils.h>
#include <price_manager/price_manager.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

void BinanceUtils::do_init()
{
    this->m_dbSourceName = BINANCE_SPOT_DB_SOURCE_NAME;
    this->m_url = BINANCE_SPOT_URL;

    //MultiMarket
    //this->m_dbSourceName = BINANCE_FUTURES_DB_SOURCE_NAME;
    //this->m_url = BINANCE_FUTURES_URL;

    init_all_symbols_in_real_net();
    // init_all_symbols_in_test_net();
    // init_all_symbols_in_binance_simulator();
}

size_t BinanceUtils::get_rounded_number(const std::string& lot_size)
{
    int pos_1 = lot_size.find_first_of("1"); // find the position of charater '1'
    return pos_1 - 1;
}

int BinanceUtils::get_lot_size_by_symbol_test_net(const std::string& symbol)
{
    return m_symbol_info_list[BINANCE_TESTNET_DB_SOURCE_NAME][symbol]["lotSize"];
}

int BinanceUtils::get_lot_size_by_symbol_real_net(const std::string& symbol)
{
    return m_symbol_info_list[this->m_dbSourceName][symbol]["lotSize"];
}

int BinanceUtils::get_round_up_price_by_symbol_test_net(const std::string& symbol)
{
    return m_symbol_info_list[BINANCE_TESTNET_DB_SOURCE_NAME][symbol]["roundUpPrice"];
}

int BinanceUtils::get_round_up_price_by_symbol_real_net(const std::string& symbol)
{
    return m_symbol_info_list[this->m_dbSourceName][symbol]["roundUpPrice"];
}

int BinanceUtils::get_lot_size_by_symbol_futures(const std::string& symbol)
{
    return m_symbol_info_list[BINANCE_FUTURES_DB_SOURCE_NAME][symbol]["lotSize"];
}

int BinanceUtils::get_round_up_price_by_symbol_futures(const std::string& symbol)
{
    return m_symbol_info_list[BINANCE_FUTURES_DB_SOURCE_NAME][symbol]["roundUpPrice"];
}

Json BinanceUtils::get_exchange_info(const std::string& url)
{
    Json exchange_info = APIHandlerBinance(nullptr).set_url(url).send_binance_normal_request("/api/v3/exchangeInfo", "");
    //Json exchange_info = APIHandlerBinanceFutures(nullptr).set_url(url).send_binance_normal_request("/fapi/v1/exchangeInfo", ""); //MultiMarket
    Json symbols = exchange_info["symbols"];
    Json res;

    symbols.for_each([this, &res](Json& symbol)
    {
        std::string symbol_name = symbol["symbol"];
        res[symbol_name] = symbol;

        res[symbol_name]["tickSize"] = std::stold((std::string&&)symbol["filters"][0]["tickSize"]);
        res[symbol_name]["lotSize"] = get_rounded_number(symbol["filters"][1]["stepSize"]);
        res[symbol_name]["roundUpPrice"] = get_rounded_number(symbol["filters"][0]["tickSize"]);
    });

    return res;
}

Json BinanceUtils::get_exchange_info_2(const std::string& url)
{
    Json exchange_info = APIHandlerBinanceFutures(nullptr).set_url(BINANCE_FUTURES_URL).send_binance_normal_request("/fapi/v1/exchangeInfo", ""); //MultiMarket
    Json symbols = exchange_info["symbols"];
    Json res;

    symbols.for_each([this, &res](Json& symbol)
    {
        std::string symbol_name = symbol["symbol"];
        res[symbol_name] = symbol;

        res[symbol_name]["tickSize"] = std::stold((std::string&&)symbol["filters"][0]["tickSize"]);
        res[symbol_name]["lotSize"] = get_rounded_number(symbol["filters"][1]["stepSize"]);
        res[symbol_name]["roundUpPrice"] = get_rounded_number(symbol["filters"][0]["tickSize"]);
    });

    return res;
}

void BinanceUtils::init_all_symbols_in_real_net()
{
    m_symbol_info_list[this->m_dbSourceName] = get_exchange_info(this->m_url);
    m_symbol_info_list[BINANCE_FUTURES_DB_SOURCE_NAME] = get_exchange_info_2(this->m_url);

    // MongoQuery query = MongoDB::instance()
    //     .set_db_and_collection(this->m_dbSourceName, "all_symbols_name");

    // if (query.count_documents() == 0)
    // {
    //     m_all_symbols_name[this->m_dbSourceName] = Json::create_array();

    //     m_symbol_info_list[this->m_dbSourceName].for_each_with_key([this](const std::string& symbol, Json& val)
    //     {
    //         m_all_symbols_name[this->m_dbSourceName].push_back(symbol);
    //     });

    //     // Save to DB
    //     Json data = {
    //         {"data", m_all_symbols_name[this->m_dbSourceName]}
    //     };
    //     query.insert_one(data);
    // }
    // else
    // {
    //     m_all_symbols_name[this->m_dbSourceName] = query.find_any()["data"];
    // }
}

void BinanceUtils::init_all_symbols_in_test_net()
{
    m_symbol_info_list[BINANCE_TESTNET_DB_SOURCE_NAME] = get_exchange_info(BINANCE_TESTNET_URL);

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(BINANCE_TESTNET_DB_SOURCE_NAME, "all_symbols_name");

    if (query.count_documents() == 0)
    {
        m_all_symbols_name[BINANCE_TESTNET_DB_SOURCE_NAME] = Json::create_array();

        m_symbol_info_list[BINANCE_TESTNET_DB_SOURCE_NAME].for_each([this](Json& symbol)
        {
            std::string symbol_name = symbol["symbol"];
            m_all_symbols_name[BINANCE_TESTNET_DB_SOURCE_NAME].push_back(symbol_name);
        });

        // Save to DB
        Json data = {
            {"data", m_all_symbols_name[BINANCE_TESTNET_DB_SOURCE_NAME]}
        };
        query.insert_one(data);
    }
    else
    {
        m_all_symbols_name[BINANCE_TESTNET_DB_SOURCE_NAME] = query.find_any()["data"];
    }
}

void BinanceUtils::init_all_symbols_in_binance_simulator()
{
    m_all_symbols_name[BINANCE_SIMULATOR_DB_SOURCE_NAME] = m_all_symbols_name[this->m_dbSourceName];
}

Json& BinanceUtils::get_symbol_info_with_exchange(const std::string& symbol)
{
    std::vector<std::string> arr = Utils::instance().split_string((std::string&&)symbol, "#");
    std::string exchange_info = arr[0];
    std::string symbol_info = arr[1];
    if (exchange_info == BINANCE_FUTURES_ABBREVIATION_NAME)
    {
        return get_symbol_info_futures(symbol_info);
    }
    else 
    {
        return get_symbol_info(symbol_info);
    }
    
}

Json& BinanceUtils::get_symbol_info(const std::string& symbol)
{
    return m_symbol_info_list[this->m_dbSourceName][symbol];
}

Json& BinanceUtils::get_symbol_info_futures(const std::string& symbol)
{
    return m_symbol_info_list[BINANCE_FUTURES_DB_SOURCE_NAME][symbol];
}

Json BinanceUtils::get_all_symbols_by_db_name(const std::string& db_name)
{
    return m_all_symbols_name[db_name];
}

Json BinanceUtils::get_24h_price_ticker(User* user)
{
    long today_0h = Utils::instance().get_0h_today_in_utc() * 1000;
    long tomorrow_0h = Utils::instance().get_0h_tomorrow_in_utc() * 1000;

    return get_price_ticker_by_date(user, today_0h, tomorrow_0h);
}

Json BinanceUtils::get_48h_price_ticker(User* user)
{
    long two_day_before_0h = Utils::instance().get_0h_by_number_of_day_before_in_utc(1) * 1000;
    long tomorrow_0h = Utils::instance().get_0h_tomorrow_in_utc() * 1000;

    return get_price_ticker_by_date(user, two_day_before_0h, tomorrow_0h);
}

Json BinanceUtils::get_price_ticker_by_date(User* user, long from, long to)
{
    bsoncxx::v_noabi::document::view_or_value filter = document{} <<
        "user_id" << user->get_user_id() <<
        "transactTime" << open_document <<
            "$gt" << from <<
            "$lte" << to <<
        close_document << finalize;

    Json price_ticker_list = MongoDB::instance()
        .set_db_and_collection(user->get_storage_source_db_name(), "price_ticker")
        .find_many(filter);

    Json res;
    price_ticker_list.for_each([&res](Json& json)
    {
        long orderId = json["orderId"];
        if (orderId != 0)
        {
            res[std::to_string(orderId)] = json;
        }
    });

    return res;
}

long double BinanceUtils::get_current_price_by_symbol(const std::string& symbol)
{
    return PriceManager::instance().get_price_by_symbol(symbol);
}

std::vector<std::string> BinanceUtils::get_asset_list(StorageSource* storage_source)
{
    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(storage_source->get_db_name(), "assets");

    // If there is no asset list in DB, fetch a new one
    Json assets = query.find_one("user_id", storage_source->get_user_id());
    if (assets.is_null() == true)
    {
        fetch_new_asset_list(storage_source);
    }

    // Query asset list from DB and return
    assets = query.find_one("user_id", storage_source->get_user_id());
    if (assets.is_null() == false && assets.has_field("data"))
    {
        std::vector<std::string> res;
        assets["data"].for_each([&res](Json& asset)
        {
            res.push_back(asset);
        });

        return res;
    }

    return std::vector<std::string>();
}

void BinanceUtils::fetch_new_asset_list(StorageSource* storage_source)
{
    std::vector<std::string> asset_list = get_asset_list_from_Binance(storage_source);
    save_asset_list_to_DB(asset_list, storage_source);
}

std::vector<std::string> BinanceUtils::get_asset_list_from_Binance(StorageSource* storage_source)
{
    std::vector<std::string> asset_list;

    Json account_info = APIHandlerBinance(nullptr)
        .set_authen_info(storage_source)
        .send_binance_request("/api/v3/account", "recvWindow=45000", RequestMethod::GET);

    account_info["balances"].for_each([&asset_list](Json& balance)
    {
        asset_list.push_back(balance["asset"]);
    });

    return asset_list;
}

void BinanceUtils::save_asset_list_to_DB(const std::vector<std::string>& asset_list, StorageSource* storage_source)
{
    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(storage_source->get_db_name(), "assets");

    // Save new asset list
    Json json;
    json["data"] = Json::create_array();
    for (int i = 0; i < asset_list.size(); i++)
    {
        json["data"].push_back(asset_list[i]);
    }
    json["user_id"] = storage_source->get_user_id();

    query.insert_one(json);
}