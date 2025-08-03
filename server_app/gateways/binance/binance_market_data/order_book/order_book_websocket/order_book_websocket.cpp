#include <gateways/binance/binance_market_data/order_book/order_book_websocket/order_book_websocket.h>
#include <json/json.h>
#include <time/timer.h>
#include <time/measure_time.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

OrderBookWebsocket::OrderBookWebsocket(const std::string& symbol, size_t depth_level, net::io_context& ioc, EventBase* event_base, std::function<void(std::string)> on_order_book_ws)
    : m_symbol{symbol}, m_depth_level{depth_level}, m_ioc{ioc}, m_event_base{event_base}, m_on_order_book_ws{on_order_book_ws}
{
    STRING_LOWER_CASE(m_symbol);
    std::string ws_path = "/ws/" + m_symbol + "@depth" + std::to_string(m_depth_level);

    std::cout << "ws path: " << ws_path << std::endl;

    m_websocket = std::make_shared<WebsocketClientAsync>(m_ioc, m_event_base, m_symbol + "_order_book_ws");
    m_websocket->set_callbacks(
        // on_connect
        [this, ws_path, websocket = m_websocket->weak_from_this()]() -> TaskVoid
        {
            std::cout << "Websocket [ws_path] is connected: " << std::endl;
            if (auto ws = websocket.lock())
            {
                // Set period time to send ping frame at every 30 seconds
                ws->add_keep_websocket_alive_task([this, websocket = std::weak_ptr<WebsocketClientAsync>(ws)]() -> TaskVoid
                {
                    if (auto ws = websocket.lock())
                    {
                        ws->send_ping();
                    }

                    co_return;
                }, CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD);
            }

            co_return;
        },
        // on_message
        [this, symbol](std::string buffer) -> TaskVoid
        {
            MeasureTime t("Depth data handle from websocket", MeasureUnit::MICROSECOND);
            m_on_order_book_ws(std::move(buffer));

            co_return;
        },
        // on_disconnect
        [this, symbol]() -> TaskVoid
        {
            // Re-start
            // ADD_LOG("Disconnect, re-start BinanceMarketData");
            // this->start_websocket(symbol);

            co_return;
        },
        // on_close
        []() -> TaskVoid
        {
            // ADD_LOG("BinanceMarketData close");
            co_return;
        }
    );

    m_websocket->connect("fstream.binance.com", "443", ws_path);
}