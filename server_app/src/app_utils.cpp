#include <app_utils.h>
#include <app_constants.h>
#include <utils.h>

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
