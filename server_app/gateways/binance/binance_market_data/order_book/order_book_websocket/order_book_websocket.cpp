#include <gateways/binance/binance_market_data/order_book/order_book_websocket/order_book_websocket.h>
#include <json/json.h>
#include <time/timer.h>
#include <time/measure_time.h>
#include <app_constants.h>
#include <mongo_db/mongo_db.h>
#include <utils/utils.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

OrderBookWebsocket::OrderBookWebsocket(const std::string& symbol, size_t depth_level, EpollBase* event_base, std::function<void(std::string)> on_order_book_ws)
    : m_symbol{symbol}, m_depth_level{depth_level}, m_event_base{event_base}, m_on_order_book_ws{on_order_book_ws}
{
    STRING_LOWER_CASE(m_symbol);
    start();
}

void OrderBookWebsocket::start()
{
    std::string ws_path = "/ws/" + m_symbol + "@depth" + std::to_string(m_depth_level) + "@100ms";

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_event_base, BINANCE_FUTURES_WS_URL, std::stoi(BINANCE_FUTURES_WS_PORT), ws_path,
        // on_connect
        [this, ws_path]() -> Task<void>
        {
            spdlog::info("OrderBookWebsocket [{}] is connected", ws_path);

            MongoDB::instance()
                .set_db_and_collection("websocket_monitoring", "OrderBookWebsocket")
                .insert_one(Json{
                    {"event", "connected"},
                    {"symbol", m_symbol},
                    {"depth_level", m_depth_level},
                    {"timestamp", Utils::get_time_now_in_string_HMS_DMY()}
                }
            );

            co_return;
        },
        // on_message
        [this](std::string buffer) -> Task<void>
        {
            m_on_order_book_ws(std::move(buffer));
            co_return;
        },
        // on_disconnect
        [this, symbol = m_symbol]() -> Task<void>
        {
            spdlog::debug("OrderBookWebsocket [{}] disconnected, re-starting...", symbol);
            // this->start();
            co_return;
        },
        // on_close
        [symbol = m_symbol]() -> Task<void>
        {
            spdlog::debug("OrderBookWebsocket [{}] closed", symbol);
            co_return;
        }
    );
}