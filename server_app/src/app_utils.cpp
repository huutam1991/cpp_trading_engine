#include <app_utils.h>
#include <app_constants.h>
#include <utils.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance.h>
#include <order_manager/order_manager.h>

ThreadPool* AppUtils::get_app_pool()
{
    static ThreadPool* app_pool = nullptr;

    if (app_pool == nullptr)
    {
        std::unique_lock lock(m_app_pool_mutex);
        if (app_pool == nullptr)
        {
            app_pool = new ThreadPool(NUMBER_OF_APP_THREADS, "App Pool");
            app_pool->set_write_log(false);
        }
    }

    return app_pool;
}

Json AppUtils::get_register_source_by_user(User* user)
{
    Json res = Json::create_array();
    std::vector<std::string> str_arr;

    for (auto it = StorageSource::m_source_map.begin(); it != StorageSource::m_source_map.end(); it++)
    {
        int count = MongoDB::instance()
            .set_db_and_collection(it->first, "info")
            .count_documents("user_id", user->get_user_id());

        if (count > 0)
        {
            str_arr = Utils::instance().split_string(it->first, "_");
            res.push_back(Json {
                {"enviroment", str_arr[1]},
                {"source", str_arr[0]},
            });
        }
    }

    return res;
}

Json AppUtils::get_balance_by_user(User* user)
{
    Json res;
    StorageSource* active_storage_source = user->get_active_storage_source().get();

    // Balance in Binance
    if (active_storage_source->get_source_type() == SourceType::BINANCE_TESTNET ||
        active_storage_source->get_source_type() == SourceType::BINANCE_SPOT)
    {
        Json account_info = APIHandlerBinance(nullptr)
            .set_authen_info(active_storage_source)
            .send_binance_request("/api/v3/account", "recvWindow=55000", RequestMethod::GET);

        if (account_info.has_field("balances"))
        {
            res = account_info["balances"];
        }
        else
        {
            res = account_info;
        }
    }

    // Balance in FTX
    {
        // TBD
    }

    return res;
}

Json AppUtils::get_symbols_in_use_by_user(User* user)
{
    std::string user_id = user->get_user_id();

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(USER_DB_NAME, "symbols_in_use");

    Json symbols = query.find_one("user_id", user_id);
    if (symbols.is_null() == true)
    {
        // If cannot find any data, return array with only 1 symbol BTCUSDT - ["BTCUSDT"]
        symbols = Json::create_array();
        symbols.push_back("BTCUSDT");

        // Then insert this symbols data to DB
        query.insert_one({
            {"user_id", user->get_user_id()},
            {"symbols", symbols}
        });
    }
    else
    {
        symbols = symbols["symbols"]; // Only return symbols list
    }

    return symbols;
}

Json AppUtils::get_trading_strategy_result(User* user, long from, long to)
{
    bsoncxx::v_noabi::document::view_or_value filter = document{} <<
        "user_id" << user->get_user_id() <<
        "hit_time" << open_document <<
            "$gt" << from <<
            "$lte" << to <<
        close_document << finalize;

    Json trading_result_list = MongoDB::instance()
        .set_db_and_collection(user->get_storage_source_db_name(), TRADING_RESULT)
        .find_many(filter);

    trading_result_list.sort([](Json& a, Json& b)
    {
        return (long)a["hit_time"] < (long)b["hit_time"];
    });

    Json res;
    trading_result_list.for_each([&res](Json& data)
    {
        // Strategy
        std::string strategy_name = data["strategy_name"];
        Json strategy = res[strategy_name];

        // Symbol
        std::string symbol_name = data["pair_name"];
        if (strategy.has_field(symbol_name) == false)
        {
            strategy[symbol_name] = Json::create_array();
        }
        strategy[symbol_name].push_back(data);
    });

    return res;
}

bool AppUtils::check_is_source_registered_by_user(const std::string& source_db_name, User* user)
{
    int count = MongoDB::instance()
        .set_db_and_collection(source_db_name, "info")
        .count_documents("user_id", user->get_user_id());

    return count > 0;
}

bool AppUtils::is_long_number(const std::string& number_str)
{
    try {
        size_t pos;
        std::stol(number_str, &pos);
        return pos == number_str.length();
    } 
    catch (const std::invalid_argument& ia) 
    {
        return false;
    } 
    catch (const std::out_of_range& oor) 
    {
        return false;
    }
}
