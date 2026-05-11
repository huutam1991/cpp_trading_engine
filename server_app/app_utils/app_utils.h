#pragma once

#include <utils/util_macros.h>
#include <utils/utils.h>
#include <app_constants.h>
#include <json/json.h>
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

public:
    static bool is_long_number(const std::string& number_str);
    static bool is_all_digit(const std::string& str);
    static OrderId parse_order_id(const std::string& str);
    static OrderId clientOrderIdToSystemOrderId(const std::string& client_order_id);
    static double round_up_quantity_by_instrument(Instrument* instrument, double quantity);
};
