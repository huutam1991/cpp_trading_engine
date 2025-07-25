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
#include <c_json/json_object.h>
// #include <c_json/json.h>

#include <app_utils/log_init.h>
#include <instrument/instrument.h>
#include <gateways/gateway_manager.h>
#include <order/order_manager.h>
#include <strategy/strategy_manager.h>

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

    // // Init DBHelper with
    // DBHelper::init(EventBaseManager::get_event_base_by_id(EventBaseID::DB_HELPER));

    // GatewayManager::instance().init();
    // OrderManager::instance().init();

    // // Strategy
    // StrategyManager::instance().init();

    // // Server
    // HttpsServer server(port, web_data_path, EventBaseManager::get_event_base_by_id(EventBaseID::APP));
    // server.start();

    for (int i = 0; i < 100; i++)
    {
        spdlog::info("Current size of JsonValuePool: {}", JsonValuePool::size());
        spdlog::info("Current size of JsonObjectPool: {}", JsonObjectPool::size());
        JsonNew b;
        b[1][6] = "Hello World";
        b[1][8] = 123;
        JsonNew c = b;

        JsonNew d = c;
        std::string e = d[1][6];
        spdlog::info("Value of e: {}", e);
        size_t f = d[1][8];
        spdlog::info("Value of f: {}", f);
        d[1][8] = 456;
        size_t g = b[1][8];
        spdlog::info("Value of g: {}", g);
        spdlog::info("Current size of JsonValuePool: {}", JsonValuePool::size());
        spdlog::info("Current size of JsonObjectPool: {}", JsonObjectPool::size());
    }
    spdlog::info("Current size of JsonValuePool: {}", JsonValuePool::size());
    spdlog::info("Current size of JsonObjectPool: {}", JsonObjectPool::size());

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
