#pragma once

#include <utils/util_macros.h>
#include <utils/utils.h>
#include <app_constants.h>
#include <c_json/json.h>
#include <thread_pool/thread_pool.h>
#include <mongo_db/mongo_db.h>
#include <coroutine/event_base_manager.h>

#include <order/order.h>
#include <instrument/instrument.h>

class AppUtils
{
    Singleton(AppUtils);

private:
    std::mutex m_AppUtils_mutex;
    std::mutex m_app_pool_mutex;

    EventBase* m_event_base = nullptr;

public:
    bool is_long_number(const std::string& number_str);
    bool is_all_digit(const std::string& str);
    OrderId parse_order_id(const std::string& str);
    static double round_up_quantity_by_instrument(Instrument* instrument, double quantity);

    EventBase* get_app_event_base();
};
