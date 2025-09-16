#include "trade_data.h"
#include <app_constants.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

BinanceTradeData::BinanceTradeData(const std::string& symbol, net::io_context& ioc, EventBase* event_base, std::function<void(std::string)> on_order_book_ws)
    : m_symbol{symbol}, m_ioc{ioc}, m_event_base{event_base}
{
    STRING_LOWER_CASE(m_symbol);
    this->start();
}

void BinanceTradeData::start()
{
    std::string ws_path = "/ws/" + m_symbol + "@aggTrade";

    m_websocket = std::make_shared<WebsocketClientAsync>(m_ioc, m_event_base, m_symbol + "_trade_data_ws");
    m_websocket->set_callbacks(
        // on_connect
        [this, ws_path, websocket = m_websocket->weak_from_this()]() -> Task<void>
        {
            spdlog::info("BinanceTradeData [{}] is connected", ws_path);
            if (auto ws = websocket.lock())
            {
                // Set period time to send ping frame at every 30 seconds
                ws->add_keep_websocket_alive_task([this, websocket = std::weak_ptr<WebsocketClientAsync>(ws)]() -> Task<void>
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
        [this](std::string buffer) -> Task<void>
        {
            // MeasureTime t("Depth data handle from websocket", MeasureUnit::MICROSECOND);
            // m_on_order_book_ws(std::move(buffer));

            co_return;
        },
        // on_disconnect
        [this, symbol = m_symbol]() -> Task<void>
        {
            // Re-start
            spdlog::debug("BinanceTradeData [{}] disconnected, re-starting...", symbol);
            this->start();

            co_return;
        },
        // on_close
        [symbol = m_symbol]() -> Task<void>
        {
            spdlog::debug("BinanceTradeData [{}] closed", symbol);
            co_return;
        }
    );

    m_websocket->connect(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT, ws_path);
}