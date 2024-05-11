#ifndef APP_UTILS_H
#define APP_UTILS_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <thread_pool.h>
#include <mongo_db/mongo_db.h>
#include <storage_source/storage_source.h>
#include <user_manager/user.h>

class AppUtils
{
    Singleton(AppUtils);

private:
    std::mutex m_AppUtils_mutex;
    std::mutex m_app_pool_mutex;

public:
    Json get_register_source_by_user(User* user);
    Json get_balance_by_user(User* user);
    Json get_symbols_in_use_by_user(User* user);
    Json get_trading_strategy_result(User* user, long from, long to);
    bool check_is_source_registered_by_user(const std::string& source_db_name, User* user);

    bool is_long_number(const std::string& number_str);
    
    ThreadPool* get_app_pool();
};

#endif //APP_UTILS_H