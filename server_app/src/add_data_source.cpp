#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include <data_feed/data_feed_binance_spot/data_feed_binance.h>
//#include <data_feed/data_feed_binance_futures/data_feed_binance_futures.h>
#include <data_feed/exchange_connector/binance_connector.h>
#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <app_constants.h>
#include <vector>

using namespace std;

void add_data_source()
{
    // Binance's websocket data user
    // (new DataSourceBinanceUser())->start();

    // follow binance spot market price 
    (new DataFeedBinance())->start();
    // (new DataFeedBinanceFutures())->start(); //MultiMarket
    
    // // vector<string> symbols = {"BTCUSDT", "ETHUSDT", "BNBUSDT", "DOTUSDT", "ARBUSDT"};
    // vector<string> symbols = {"BTCUSDT", "ETHUSDT", "ETHBTC"};
    // // vector<string> symbols = {"BTCUSDT"};

    // int depth = 5;

    // for(auto symbol : symbols)
    // {
    //     // create order book
    //     OrderBookManager::instance().add_order_book(BINANCE_SPOT_ABBREVIATION_NAME + "#" + (string&&)symbol, 100);

    //     BinanceConnector* binance_connector = 
    //         new BinanceConnector(BINANCE_SPOT_WS_URL, 
    //                             BINANCE_SPOT_WS_PORT,
    //                             BINANCE_SPOT_ABBREVIATION_NAME);
    //         // new BinanceConnector(BINANCE_FUTURES_WS_URL, 
    //         //                     BINANCE_FUTURES_WS_PORT,
    //         //                     BINANCE_FUTURES_ABBREVIATION_NAME);
    //     binance_connector->feed_order_book(symbol, depth);
    // }

    if (const char* init_price_pending_time = std::getenv("INIT_PRICE_PENDING_TIME"))
    {
        int pending_time_int = std::stoi(std::string(init_price_pending_time));
        std::this_thread::sleep_for (std::chrono::seconds(pending_time_int));
    }

    // Binance's websocket data ticker
    // (new DataSourceBinanceTicker("ETHBTC"))->start();
    // (new DataSourceBinanceTicker("BTCUSDT"))->start();
    // (new DataSourceBinanceTicker("ETHUSDT"))->start();

    // new MarketScanning("ETHBTC", "BTCUSDT", "ETHUSDT");
}