#include <iostream>
#include <filesystem>
#include <string>
// #include <glog/logging.h>

#include <utils/constants.h>
#include <app_constants.h>
#include <network/websocket/websocket_client_async.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <time/timer.h>
#include <network/external_request/https_client_async.h>
#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>
#include <cache/cache_pool.h>
#include <cache/share_string.h>
#include <json/json_object.h>
// #include <json/json.h>

#include <app_utils/log_init.h>
#include <instrument/instrument.h>
#include <gateways/gateway_manager.h>
#include <order/order_manager.h>
#include <order/simulator_order.h>
#include <strategy/strategy_manager.h>

#include <system_io/https_server_io/https_server_socket.h>
#include <coroutine/epoll_base.h>

#include <network/https_client_request/https_client_request.h>

extern void add_app_route();
extern void add_bad_request();

Task<void> test_https_client_request(EpollBase* epoll_base)
{
    HttpsClientRequest https_client_request(epoll_base, "1.1.1.1", 443);

    while (true)
    {
        {
            MeasureTime mt("GET 1.1.1.1/cdn-cgi/trace", MeasureUnit::MILLISECOND);
            HttpsClientResponse response_get = co_await https_client_request.get("/cdn-cgi/trace");
            spdlog::info("GET 1.1.1.1/cdn-cgi/trace response: {} - {}", response_get.status_code, response_get.body);
        }

        co_await Timer::sleep_for(1000);
    }
}

int main(int argc, char **argv) {

    const int port = atoi(argv[1]);
    const char* web_data_path = argv[2];

    // Initialize Google’s logging library
    // google::InitGoogleLogging(argv[0]);

    if (!std::filesystem::is_directory(std::filesystem::path(web_data_path))) {
        // // LOG(ERROR) << "Invalid web data directory\n";
        return EXIT_FAILURE;
    }

    // Init JWTManager: issuer + expried time + secret key
    JWTManager::instance()
        .set_issuer(TOKEN_ISSUER)
        .set_expried_time(TOKEN_EXPRIED_TIME)
        .set_secret_key(TOKEN_SECRET_KEY);

    // Init routes
    add_app_route();
    add_bad_request();

    // Init SpdLog format
    LogInit::init();

    // GatewayManager::instance().init();
    // OrderManager::instance().init();
    // SimulatorOrder::init();

    // // Strategy
    // StrategyManager::instance().init();

    // Start HTTPS server - running on EpollBase
    EpollBase* epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::SYSTEM_IO_TASK);
    // HttpsServerSocket* https_server_object = new HttpsServerSocket(port);
    // epoll_base->start_living_system_io_object(https_server_object);

    // Test HTTPS client request
    test_https_client_request(epoll_base).start_running_on(epoll_base);

    // Main loop, only sleep here
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
