#include <data_feed/exchange_connector/binance_connector.h>
#include <data_feed/data_processor/data_parser/binance_parser.h>
#include <data_feed/data_processor/data_parser/binance_future_parser.h>
#include <data_feed/data_processor/data_storage/order_book_manager.h>

#include <utils.h>
#include <algorithm>

BinanceConnector::BinanceConnector(const string binance_ws_url, 
                    const string binance_ws_port, 
                    const string exchange_name) :
    m_binance_ws_url(binance_ws_url),
    m_binance_ws_port(binance_ws_port),
    m_exchange_name(exchange_name)
{
    m_wsh = nullptr;
}

BinanceConnector::~BinanceConnector()
{
    if (m_wsh)
        m_wsh->close();
}

size_t BinanceConnector::get_feed_order_book_id()
{
    static int feed_order_book_id = 0;
    std::unique_lock lock(m_feed_order_book_mutex);

    return ++feed_order_book_id;
}

void BinanceConnector::feed_order_book(const string symbol, const int depth)
{
    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, 
                                                    m_binance_ws_port, 
                                                    "/ws");
    m_websocket->set_use_valid_data(true);

    m_websocket->on_connect([this, symbol, depth](WebsocketClientHandle& ws)
    {
        m_wsh = &ws;
        ADD_LOG("Binance websocket connected");

        // prepare sid
        int sid = get_feed_order_book_id();

        // make symbol to lower case
        string binance_symbol = symbol;        
        STRING_LOWER_CASE(binance_symbol);

        Json params;
        params[0] = binance_symbol + "@bookTicker";
        if (depth == 5 || depth == 10 || depth == 20) {
            params[1] = binance_symbol + "@depth" + to_string(depth) + "@100ms";
        }
        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = sid;

        ws.write(subcribe.get_string_value());
    });

    m_websocket->on_message([this, symbol](const std::string& buffer, WebsocketClientHandle& ws)
    {
        static int counter = 0;

        Json json = Json();
        if (m_exchange_name == BINANCE_SPOT_ABBREVIATION_NAME) 
        {
            BinanceParser binance_parser = BinanceParser(&json, m_exchange_name, symbol);
            binance_parser.parse_order_book(buffer);
        }
        else 
        {
            BinanceFutureParser binance_parser = BinanceFutureParser(&json, m_exchange_name, symbol);
            binance_parser.parse_order_book(buffer);
        }

        counter++;
        if (counter % 5000 == 0)
            ADD_LOG("Binance: " << counter);
    
        // update to ring buffer
        OrderBookManager::instance().update_order_book(json);

    });

    m_websocket->on_close([this, symbol, depth](websocket::close_code close_code)
    {
        m_wsh = nullptr;
        ADD_LOG("Stream current_price close, close_code = " << close_code);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start feed order book");
            this->feed_order_book(symbol, depth);
        }
    });

    m_websocket->run();
}
