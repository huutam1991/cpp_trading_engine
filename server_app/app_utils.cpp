#include <app_utils.h>
#include <app_constants.h>
#include <utils/utils.h>

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

EventBase* AppUtils::get_app_event_base()
{
    if (m_event_base == nullptr)
    {
        std::unique_lock lock(m_app_pool_mutex);
        if (m_event_base == nullptr)
        {
            m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::APP);
        }
    }

    return m_event_base;
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

// Method to check if a string contains all of digits
// (Some orders placed manually using Iphone has [clientOrderId] like this: "ios_47d0a66fc34f421d8f56e4d4048bc8d4")
// (Which cause error when force cast to std::stoull)
bool AppUtils::is_all_digit(const std::string& str)
{
    for (char c : str)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }

    return true;
}

OrderId AppUtils::parse_order_id(const std::string& str)
{
    if (is_all_digit(str) == false)
    {
        return 0;
    }

    return std::stoull(str);
}
