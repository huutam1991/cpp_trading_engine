#include <iostream>
#include <filesystem>
#include <string>
// #include <glog/logging.h>

#include <utils/constants.h>
#include <app_constants.h>
#include <https_server/http_server.h>
#include <https_server/https_server.h>
#include <websocket/websocket_client_async.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <time/timer.h>
#include <external_request/https_client_async.h>
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

#include <system_io/http_server_socket.h>
#include <coroutine/epoll_base.h>

extern void add_app_route();
extern void add_bad_request();

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

    // // Init Timer with ioc TIMER
    // Timer::init(IOCPool::get_ioc_by_id(IOCId::TIMER));

    // Init DBHelper with
    // DBHelper::init(EventBaseManager::get_event_base_by_id(EventBaseID::SYSTEM_INFRASTRUCTURE));

    // GatewayManager::instance().init();
    // OrderManager::instance().init();
    // SimulatorOrder::init();

    // // Strategy
    // StrategyManager::instance().init();

    // Server
    // HttpsServer server(port, web_data_path, EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY));
    // server.start();

    EpollBase epoll_base;
    HttpServerSocket* http_server_object = new HttpServerSocket(8080);
    epoll_base.start_running_system_io_object(http_server_object);

    epoll_base.loop();

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
