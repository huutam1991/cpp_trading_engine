#include <iostream>
#include <filesystem>
#include <glog/logging.h>

#include <constants.h>
#include <app_constants.h>
#include <http_server.h>
#include <https_server.h>
#include <websocket/websocket_client_async.h>
#include <websocket/websocket_server.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>

#include <gateways/gateway_manager.h>
#include <order/order_manager.h>
#include <strategy/strategy.h>

extern void add_app_route();
extern void add_bad_request();

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

    net::io_context ioc;

    auto client = std::make_shared<WebsocketClientAsync>(ioc);
    client->set_on_message([&client](std::string message)
    {
        ADD_LOG("on message: " << message);
    });
    client->set_on_disconnect([&client]()
    {
        ADD_LOG("Websocket close as normal");
    });
    client->connect("echo.websocket.events", "80"); // Public echo server

    net::steady_timer timer1(ioc, std::chrono::seconds(1));
    timer1.async_wait([client](auto) {
        client->send("Hello WebSocket: 1");
    });
    net::steady_timer timer2(ioc, std::chrono::seconds(2));
    timer2.async_wait([client](auto) {
        client->send("Hello WebSocket: 2");
    });
    net::steady_timer timer3(ioc, std::chrono::seconds(3));
    timer3.async_wait([client](auto) {
        client->send("Hello WebSocket: 3");
    });
    net::steady_timer timer4(ioc, std::chrono::seconds(4));
    timer4.async_wait([client](auto) {
        client->send("Hello WebSocket: 4");
        client->close();
    });

    ioc.run();

    // GatewayManager::instance().init();
    // OrderManager::instance().init();
    // Strategy::instance().init();

    // // Server
    // HttpsServer server(port, web_data_path);
    // server.start();

    LOG(INFO) << "Main exit" << std::endl;

    return EXIT_SUCCESS;
}
