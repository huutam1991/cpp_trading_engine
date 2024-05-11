#include <list>

#include <external_request/external_request_ssl.h>
#include <mongo_db/mongo_db.h>
#include <back_testing/back_testing.h>
#include <order_manager/order_manager.h>

bool BackTesting::is_start()
{
    return m_is_start;
}

bool BackTesting::is_back_testing_mode()
{
    return m_is_back_testing_mode;
}

long BackTesting::register_callback_back_testing_mode(std::function<void(bool)> callback)
{
    static long callback_id = 1;

    std::unique_lock lock(m_back_testing_mode_mutex);
    m_back_testing_mode_callback_list.insert(std::make_pair(callback_id, callback));

    return callback_id++;
}

void BackTesting::unregister_callback_back_testing_mode(long callback_id)
{
    std::unique_lock lock(m_back_testing_mode_mutex);

    if (m_back_testing_mode_callback_list.find(callback_id) != m_back_testing_mode_callback_list.end())
    {
        m_back_testing_mode_callback_list.erase(callback_id);
    }
}

long BackTesting::register_callback_back_testing_start(std::function<void(bool)> callback)
{
    static long callback_id = 1;

    std::unique_lock lock(m_back_testing_start_mutex);
    m_back_testing_start_callback_list.insert(std::make_pair(callback_id, callback));

    return callback_id++;
}

void BackTesting::unregister_callback_back_testing_start(long callback_id)
{
    std::unique_lock lock(m_back_testing_start_mutex);

    if (m_back_testing_start_callback_list.find(callback_id) != m_back_testing_start_callback_list.end())
    {
        m_back_testing_start_callback_list.erase(callback_id);
    }
}

Json BackTesting::send_request_to_binance_simulator(const std::string& path, RequestMethod method, const Json& body)
{
    std::string res_str = ExternalRequestSsl(m_binance_simulator_url, m_binance_simulator_port, path, method).send_request(body.get_string_value());
    return Json::parse(res_str);
}

bool BackTesting::connect_binance_simulator()
{
    Json connect_result = send_request_to_binance_simulator("/connect_binance_simulator", RequestMethod::GET);
    ADD_LOG("connect_result = " << connect_result);

    bool is_connected = false;
    if (connect_result.has_field("status") && connect_result["status"] == "OK")
    {
        is_connected = true;
    }

    return is_connected;
}

void BackTesting::set_speed_time(double speed_time)
{
    if (m_speed_time != speed_time)
    {
        Json body = {
            {"speed_time", speed_time}
        };
        send_request_to_binance_simulator("/set_speed_time", RequestMethod::POST, body);

        m_speed_time = speed_time;
    }
}

void BackTesting::set_db_name(const std::string& db_name)
{
    if (m_db_name != db_name)
    {
        Json body = {
            {"db_name", db_name}
        };
        send_request_to_binance_simulator("/set_db_name", RequestMethod::POST, body);

        m_db_name = db_name;
    }
}

void BackTesting::clean_back_testing_data()
{
    MongoDB::instance().drop_collection(BINANCE_SIMULATOR_DB_SOURCE_NAME, ORDER);
    MongoDB::instance().drop_collection(BINANCE_SIMULATOR_DB_SOURCE_NAME, EXECUTION_REPORT);
    MongoDB::instance().drop_collection(BINANCE_SIMULATOR_DB_SOURCE_NAME, PRICE_TICKER);
    MongoDB::instance().drop_collection(BINANCE_SIMULATOR_DB_SOURCE_NAME, TRADE_ERROR);
    MongoDB::instance().drop_collection(BINANCE_SIMULATOR_DB_SOURCE_NAME, TRADING_RESULT);

    OrderManager::instance().clean_all_cached_orders();
}

double BackTesting::get_speed_time()
{
    return m_speed_time;
}

std::string BackTesting::get_db_name()
{
    return m_db_name;
}

Json BackTesting::get_db_name_list()
{
    Json result = send_request_to_binance_simulator("/get_db_name_list", RequestMethod::GET);
    return result["db_name_list"];
}

Json BackTesting::get_collection_name_list_by_db_name(const std::string& db_name)
{
    Json result = send_request_to_binance_simulator("/get_collection_name_list?db_name=" + db_name , RequestMethod::GET);
    return result["collection_name_list"];
}

bool BackTesting::start_back_testing_mode()
{
    if (m_is_back_testing_mode == false)
    {
        // Check Binance Simulator service is available
        if (connect_binance_simulator() == true)
        {
            m_is_back_testing_mode = true;

            // Invoke callback
            invoke_back_testing_mode_callback_list();
        }
    }

    return m_is_back_testing_mode;
}

bool BackTesting::stop_back_testing_mode()
{
    if (m_is_back_testing_mode == true)
    {
        m_is_back_testing_mode = false;

        // Stop back testing
        set_start(false);

        // Invoke callback
        invoke_back_testing_mode_callback_list();
    }

    return m_is_back_testing_mode;
}

void BackTesting::set_start(bool is_start)
{
    if (m_is_start != is_start)
    {
        m_is_start = is_start;

        if (is_start == true)
        {
            start_test();
        }
        else
        {
            stop_test();
        }
    }
}

Json BackTesting::start_test()
{
    Json response = send_request_to_binance_simulator("/start_back_testing", RequestMethod::GET);

    // Invoke callback
    invoke_back_testing_start_callback_list();

    return response;
}

Json BackTesting::stop_test()
{
    Json response = send_request_to_binance_simulator("/stop_back_testing", RequestMethod::GET);

    // Invoke callback
    invoke_back_testing_start_callback_list();

    return response;
}

void BackTesting::invoke_back_testing_mode_callback_list()
{
    std::unique_lock lock(m_back_testing_mode_mutex);

    std::list<std::function<void(bool)>*> callback_pointer_list;
    for (auto& it: m_back_testing_mode_callback_list)
    {
        callback_pointer_list.push_back(&it.second);
    }

    lock.unlock();

    while (callback_pointer_list.size() > 0)
    {
        std::function<void(bool)>* callback = callback_pointer_list.back();
        (*callback)(m_is_back_testing_mode);
        callback_pointer_list.pop_back();
    }
}

void BackTesting::invoke_back_testing_start_callback_list()
{
    std::unique_lock lock(m_back_testing_start_mutex);

    std::list<std::function<void(bool)>*> callback_pointer_list;
    for (auto& it: m_back_testing_start_callback_list)
    {
        callback_pointer_list.push_back(&it.second);
    }

    lock.unlock();

    while (callback_pointer_list.size() > 0)
    {
        std::function<void(bool)>* callback = callback_pointer_list.back();
        (*callback)(m_is_start);
        callback_pointer_list.pop_back();
    }
}