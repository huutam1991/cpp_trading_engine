#include <timer.h>
#include <util_macros.h>

// #include <market_scanning/market_scanning.h>
#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/data_processor/data_storage/order_book.h>
#include <ring_buffer/ring_buffer.h>
#include <app_constants.h>

#include <vector>

using namespace std;

void scan_market_data()
{
    // vector<string> symbols = {"BTCUSDT", "ETHUSDT", "BNBUSDT", "DOTUSDT", "ARBUSDT"};
    vector<string> symbols = {"BTCUSDT", "ETHUSDT", "ETHBTC"};

    for(string symbol : symbols)
    {
        // string s = BINANCE_FUTURES_ABBREVIATION_NAME + "#" + symbol;
        string s = BINANCE_SPOT_ABBREVIATION_NAME + "#" + symbol;
        shared_ptr<OrderBook> order_book = 
            OrderBookManager::instance().get_order_book_by_symbol(s);
        
        if (order_book != nullptr)
        {
            order_book->begin_read_ring_buffer();
            RingBuffer<Json>* ring_buffer = order_book->read_ring_buffer();
            Json&& json_max = (*ring_buffer)[0];
            long double bid_max = 0;
            for (int i = 0; i < ring_buffer->size(); i++)
            {
                Json&& json = (*ring_buffer)[i];
                if ((long double)(json["b"]) > bid_max) 
                {
                    bid_max = (long double)(json["b"]);
                    json_max = json;
                }
            }
            
            ADD_LOG("scanning " << s << " max bid = " << json_max.get_string_value());

            order_book->end_read_ring_buffer();
        }
    }
}

void add_schedule_task()
{
    // std::string res1 = ExternalRequestSsl("api.binance.com", "443", "/api/v3/ticker/24hr?symbol=BNBBTC", RequestMethod::GET).send_request("");
    // std::string res2 = ExternalRequestSsl("api.binance.com", "443", "/api/v3/ticker/price?symbol=BNBBTC", RequestMethod::GET).send_request("");
    // std::string res3 = ExternalRequestSsl("api.binance.com", "443", "/api/v3/ticker/price?symbol=BNBETH", RequestMethod::GET).send_request("");

    // ADD_LOG("24h Price Change = " << res1);
    // ADD_LOG("Price = " << res2);
    // ADD_LOG("Price = " << res3);

    Timer::instance().add_schedule_task([]()
    {
        // ADD_LOG("schedule tasks");
        // MarketScanning::instance().start();
        scan_market_data();
    }, 50);
}
