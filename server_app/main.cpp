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
#include <c_json/json.h>

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

    // Example usage of CachePool>

    {
        ShareString str1 = std::string("Tam Nguyen");
        ShareString str2 = str1;
        ShareString str3(std::string("Hello World"));
        spdlog::info("str3: {}", str3);
        spdlog::info("size of string pool: {}", StringPool::size());

        str3 = str2;
        auto str4 = str3.from_substr(0, 3);
        spdlog::info("str1: {}", str1);
        spdlog::info("str2: {}", str2);
        spdlog::info("str3: {}", str3);
        spdlog::info("str4: {}", str4);
        spdlog::info("size of string pool: {}", StringPool::size());

        for (size_t i = 0; i < 20; i++)
        {
            {
                auto str6 = ShareString(std::string("New String " + std::to_string(i)));
            }
        }
    }
    spdlog::info("size of string pool: {}", StringPool::size());

    JsonNew a;
    // a[1] = 123;

    // int b = a;
    // spdlog::info("Value of b: {}", b);
    // long c = a;
    // spdlog::info("Value of c: {}", c);
    // size_t d = a;
    // spdlog::info("Value of d: {}", d);
    // uint32_t e = a;
    // spdlog::info("Value of e: {}", e);

    // for (int i = 0; i < 10; i++)
    // {
    //     uint32_t f = a[1];
    //     spdlog::info("Value of f1: {}", f);
    // }

    // a = "123";
    spdlog::debug("a address: {}", (size_t)&a);
    a[3] = 123;
    uint32_t f = a[1];
    spdlog::info("Value of f2: {}", f);

    // for (int i = 0; i < 10; i++)
    // {
    //     std::string f = a[2];
    //     spdlog::info("Value of f2: {}", f);
    // }

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
