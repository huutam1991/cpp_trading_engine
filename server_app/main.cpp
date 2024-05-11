#include <iostream>
#include <filesystem>
#include <glog/logging.h>

#include <constants.h>
#include <app_constants.h>
#include <http_server.h>
#include <https_server.h>
#include <websocket/websocket_server.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <timer.h>
#include <user_manager/user_manager.h>
#include <binance_utils.h>
#include <price_manager/price_manager.h>
#include <exchanges/exchange_gateway.h>

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

#include <strategy_engine/scanning_strategies_manager.h>
#include <asset_manager/binance_asset_manager.h>

extern void add_app_route();
extern void add_bad_request();

extern void add_schedule_task();
extern void add_data_source();

extern void testRingBufferWithInt();
extern void testRingBufferWithJson();
extern void testMarketScanningStrategy();
extern void testExchangeGateway();

int main(int argc, char **argv) {

    const int port = atoi(argv[1]);
    const char* web_data_path = argv[2];

    // Initialize Google’s logging library
    // google::InitGoogleLogging(argv[0]);

    if (!std::filesystem::is_directory(std::filesystem::path(web_data_path))) {
        LOG(ERROR) << "Invalid web data directory\n";
        return EXIT_FAILURE;
    }

    // Init JWTManager: issuer + expried time + secret key
    JWTManager::instance()
        .set_issuer(TOKEN_ISSUER)
        .set_expried_time(TOKEN_EXPRIED_TIME)
        .set_secret_key(TOKEN_SECRET_KEY);

    // Init Timer
    Timer::instance().init();

    add_app_route();
    add_bad_request();

    add_data_source();

    // // BinanceUtils init
    BinanceUtils::instance().do_init();

    // // PriceManager init
    PriceManager::instance().init();

    // add_schedule_task();

    APIHandlerBinance::synchronize_server_time();

    // Websocket server
    WebSocketServerType::instance().set_info("0.0.0.0", "8081");
    WebSocketServerType::instance().set_available_channel_name({
        CHANNEL_ORDER_STATUS,
        CHANNEL_PROFIT,
        CHANNEL_BALANCE,
        CHANNEL_AUTO_TRADE
    });
    WebSocketServerType::instance().set_available_common_channel_name({
        CHANNEL_SCANNING_MARKET,
        CHANNEL_SCANNING_MARKET_NOTIFICATION,
        CHANNEL_TRADE_HISTORY,
        CHANNEL_POSITION_UPDATE
    });
    WebSocketServerType::instance().set_minor_channel_name({
        CHANNEL_SCANNING_MARKET,
        CHANNEL_SCANNING_MARKET_NOTIFICATION
    });
    WebSocketServerType::instance().start();

    ExchangeGateWay::instance().initialize();

    // User init
    UserManager::instance().init();

    // init trade manager
    BinanceAssetManager::instance().init();

    // Scanning market - single
    ScanningStrategiesManager::instance().start_by_config_from_DB();

    // Server
    HttpsServer server(port, web_data_path);
    server.start();

    LOG(INFO) << "Main exit" << std::endl;

    return EXIT_SUCCESS;
}
