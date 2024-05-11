#ifndef BACK_TESTING_H
#define BACK_TESTING_H

#include <cstdlib>
#include <functional>
#include <mutex>

#include <util_macros.h>
#include <json/json.h>
#include <app_constants.h>

class BackTesting
{
    Singleton(BackTesting)

private:
    std::string m_binance_simulator_url  = std::getenv("BINANCE_SIMULATOR_URL") ?
                                           std::getenv("BINANCE_SIMULATOR_URL") :
                                           BINANCE_SIMULATOR_URL;
    std::string m_binance_simulator_port = BINANCE_SIMULATOR_PORT;
    bool        m_is_back_testing_mode   = false;
    bool        m_is_start               = false;
    double      m_speed_time             = 1;
    std::string m_db_name                = "";

    // Back Testing mode callback list
    std::mutex m_back_testing_mode_mutex;
    std::unordered_map<long, std::function<void(bool)>> m_back_testing_mode_callback_list;
    void invoke_back_testing_mode_callback_list();

    // Back Testing start callback list
    std::mutex m_back_testing_start_mutex;
    std::unordered_map<long, std::function<void(bool)>> m_back_testing_start_callback_list;
    void invoke_back_testing_start_callback_list();

    Json send_request_to_binance_simulator(const std::string& path, RequestMethod method, const Json& body = JsonNull());

public:
    // Config method
    void set_start(bool is_start);
    void set_speed_time(double speed_time);
    void set_db_name(const std::string& db_name);
    void clean_back_testing_data();
    Json start_test();
    Json stop_test();
    bool start_back_testing_mode();
    bool stop_back_testing_mode();

    // Callback Back Testing mode
    long register_callback_back_testing_mode(std::function<void(bool)> callback);
    void unregister_callback_back_testing_mode(long callback_id);

    // Callback Back Testing start
    long register_callback_back_testing_start(std::function<void(bool)> callback);
    void unregister_callback_back_testing_start(long callback_id);

    // Get status method
    bool connect_binance_simulator();
    bool is_start();
    double get_speed_time();
    std::string get_db_name();
    Json get_db_name_list();
    Json get_collection_name_list_by_db_name(const std::string& db_name);
    bool is_back_testing_mode();
};

#endif //BACK_TESTING_H
