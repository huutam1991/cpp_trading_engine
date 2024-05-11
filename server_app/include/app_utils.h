#ifndef APP_UTILS_H
#define APP_UTILS_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <thread_pool.h>
#include <mongo_db/mongo_db.h>

class AppUtils
{
    Singleton(AppUtils);

private:
    std::mutex m_AppUtils_mutex;
    std::mutex m_app_pool_mutex;

public:
    bool is_long_number(const std::string& number_str);

    ThreadPool* get_app_pool();
};

#endif //APP_UTILS_H