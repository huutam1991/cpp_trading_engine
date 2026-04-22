#include <iostream>
#include <filesystem>
#include <string>
// #include <glog/logging.h>

#include <utils/constants.h>
#include <app_constants.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <time/timer.h>
#include <network/external_request/https_client_async.h>
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
#include <system_io/https_websocket_server_io/http_websocket_server.h>
#include <coroutine/epoll_base.h>

#include <network/https_client_request/https_client_request.h>
#include <network/https_client_websocket/https_client_websocket.h>

extern void add_app_route();
extern void add_bad_request();

Task<void> test_https_client_request(EpollBase* epoll_base)
{
    while (true)
    {
        {
            HttpsClientRequest https_client_request(epoll_base, "fapi.binance.com", 443);
            MeasureTime mt("GET fapi.binance.com/fapi/v1/exchangeInfo", MeasureUnit::MILLISECOND);
            HttpsClientResponse response_get = co_await https_client_request.get("/fapi/v1/exchangeInfo");
            spdlog::info("GET fapi.binance.com/fapi/v1/exchangeInfo response: {} - {}", response_get.status_code, response_get.body);
        }

        co_await Timer::sleep_for(1000);
    }
}

Task<void> test_https_client_request_httpbin(EpollBase* epoll_base)
{
    HttpsClientRequest https_client_request(epoll_base, "httpbin.org", 443);

    while (true)
    {
        {
            MeasureTime mt("GET httpbin.org/stream/5", MeasureUnit::MILLISECOND);
            HttpsClientResponse response_get = co_await https_client_request.get("/stream/5");
            spdlog::info("GET httpbin.org/stream/5 response: {} - {}", response_get.status_code, response_get.body);
        }

        co_await Timer::sleep_for(1000);
    }
}

Task<void> test_https_client_websocket(EpollBase* epoll_base)
{
    // stream.binance.com:9443/ws/btcusdt@trade
    HttpsClientWebsocket websocket(
        epoll_base,
        "stream.binance.com",
        9443,
        "/ws/btcusdt@trade",
        []() -> Task<void>
        {
            spdlog::info("Websocket connected");
            co_return;
        },
        [](std::string message) -> Task<void>
        {
            spdlog::info("Received ws.ifelse.io's websocket message: {}", message);
            co_return;
        },
        []() -> Task<void>
        {
            spdlog::info("Websocket disconnected");
            co_return;
        },
        []() -> Task<void>
        {
            spdlog::info("Websocket closed");
            co_return;
        }
    );

    while (true)
    {
        co_await Timer::sleep_for(15000);
    }

    co_return;
}

Future<Json> get_number_future()
{
    return Future<Json>([](Future<Json>::FutureValue* value)
    {
        // Simulate some async operation
        Json result;
        result["number"] = 42; // Example data
        value->set_value(result);
    });
}

Task<void> test_future()
{
    while (true)
    {
        Json number = co_await get_number_future();
        spdlog::info("Received number from future: {}", number);

        co_await Timer::sleep_for(2000);
    }

    co_return;
}

int main(int argc, char **argv) {

    const int port = atoi(argv[1]);

    // Initialize Google’s logging library
    // google::InitGoogleLogging(argv[0]);

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

    HttpWebsocketServer* http_websocket_server_object = new HttpWebsocketServer(port);
    epoll_base->start_living_system_io_object(http_websocket_server_object);

    // Test HTTPS client request
    // test_https_client_websocket(epoll_base).start_running_on(epoll_base);
    // test_future().start_running_on(epoll_base);

    // Main loop, only sleep here
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
