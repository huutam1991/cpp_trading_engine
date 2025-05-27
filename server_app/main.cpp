#include <iostream>
#include <filesystem>
#include <glog/logging.h>

#include <utils/constants.h>
#include <app_constants.h>
#include <https_server/http_server.h>
#include <https_server/https_server.h>
#include <websocket/websocket_client_async.h>
#include <websocket/websocket_server.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <time/timer.h>

#include <gateways/gateway_manager.h>
#include <order/order_manager.h>
#include <strategy/strategy.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <strategy_mean_reversion/strategy_mean_reversion.h>

#include <external_request/https_client_async.h>
#include <ioc_pool.h>

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

    // Init Timer with ioc TIMER
    Timer::init(IOCPool::get_ioc_by_id(IOCId::TIMER));

    GatewayManager::instance().init();
    OrderManager::instance().init();

    // // Strategy
    // // Strategy::instance().init();
    StrategyPriceArbitrage::instance().init();
    // // StrategyMeanReversion::instance().init();

    // Server
    HttpsServer server(port, web_data_path, EventBaseManager::get_event_base_by_id(EventBaseID::APP));
    server.start();

    ADD_LOG("Main exit");

    return EXIT_SUCCESS;
}
