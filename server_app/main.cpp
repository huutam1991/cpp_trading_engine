#include <iostream>
#include <filesystem>
// #include <glog/logging.h>
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <utils/constants.h>
#include <app_constants.h>
#include <https_server/http_server.h>
#include <https_server/https_server.h>
#include <websocket/websocket_client_async.h>
#include <websocket/websocket_server.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <time/timer.h>
#include <external_request/https_client_async.h>
#include <ioc_pool.h>
#include <coroutine/event_base_manager.h>

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

    // // Websocket server
    // WebSocketServerType::instance().set_info("0.0.0.0", "8081");
    // WebSocketServerType::instance().set_available_channel_name({
    //     CHANNEL_ORDER_STATUS,
    //     CHANNEL_PROFIT,
    //     CHANNEL_BALANCE,
    //     CHANNEL_AUTO_TRADE
    // });
    // WebSocketServerType::instance().set_available_common_channel_name({
    //     CHANNEL_SCANNING_MARKET,
    //     CHANNEL_SCANNING_MARKET_NOTIFICATION,
    //     CHANNEL_TRADE_HISTORY,
    //     CHANNEL_POSITION_UPDATE
    // });
    // WebSocketServerType::instance().set_minor_channel_name({
    //     CHANNEL_SCANNING_MARKET,
    //     CHANNEL_SCANNING_MARKET_NOTIFICATION
    // });
    // WebSocketServerType::instance().start();

    // Init SpdLog format
    // auto console = spdlog::stdout_color_mt("console");
    // console->set_level(spdlog::level::trace);
    // spdlog::set_default_logger(console); 

    auto async_logger = spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>("async_logger");
    async_logger->set_pattern("%d-%m-%Y %H:%M:%S %^%l%$ %v");
    async_logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(async_logger);

    // Init Timer with ioc TIMER
    Timer::init(IOCPool::get_ioc_by_id(IOCId::TIMER));

    // Init DBHelper with 
    DBHelper::init(EventBaseManager::get_event_base_by_id(EventBaseID::DB_HELPER));

    GatewayManager::instance().init();
    OrderManager::instance().init();

    // // Strategy
    StrategyManager::instance().init();

    // Server
    HttpsServer server(port, web_data_path, EventBaseManager::get_event_base_by_id(EventBaseID::APP));
    server.start();

    ADD_LOG("Main exit");

    return EXIT_SUCCESS;
}
