#include "trade_data.h"
#include <app_constants.h>
#include <json/json.h>
#include <utils/utils.h>
#include <mongo_db/mongo_db.h>

#include <strategy/strategy_manager.h>

#define CHECK_KEEP_WEBSOCKET_ALIVE_PERIOD 30000

BinanceTradeData::BinanceTradeData(const std::string& symbol, EpollBase* event_base)
    : m_symbol{symbol}, m_event_base{event_base}
{
    STRING_LOWER_CASE(m_symbol);
    m_instrument = Instrument::get_instrument_by_exchange_symbol(ExchangeId::BINANCE, InstrumentType::PERPETUAL, symbol);
    start();
}

void BinanceTradeData::start()
{
    // Check to close current websocket if any
    if (m_websocket != nullptr)
    {
        m_websocket = nullptr;
    }

    std::string ws_path = "/ws/" + m_symbol + "@aggTrade";

    std::string name = m_symbol + "_trade_data_ws";

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_event_base, BINANCE_FUTURES_WS_URL, std::stoi(BINANCE_FUTURES_WS_PORT), ws_path,
        // on_connect
        [this, ws_path]() -> Task<void>
        {
            spdlog::info("BinanceTradeData [{}] is connected", ws_path);

            MongoDB::instance()
                .set_db_and_collection("websocket_monitoring", "TradeDataWebsocket")
                .insert_one(Json{
                    {"event", "CONNECTED"},
                    {"symbol", m_symbol},
                    {"timestamp", Utils::get_time_now_in_string_HMS_DMY()}
                }
            );

            co_return;
        },
        // on_message
        [this](std::string buffer) -> Task<void>
        {
            Json data = Json::parse(buffer);

            if (data.has_field("e") && data["e"] == "aggTrade")
            {
                TradeUpdate update;
                update.instrument = m_instrument;
                update.price = std::stod((std::string)data["p"]);
                update.quantity = std::stod((std::string)data["q"]);
                update.trade_id = data["a"];
                update.timestamp = (size_t)data["T"] * 1000000; // Convert milliseconds to nanoseconds
                update.is_buy = data["m"] == true; // If m is true, then the buyer is the market maker

                StrategyUpdateData data{update};

                // Public data to strategies
                StrategyManager::instance().public_data(data);
            }

            co_return;
        },
        // on_disconnect
        [this, symbol = m_symbol]() -> Task<void>
        {
            // Re-start
            spdlog::debug("BinanceTradeData [{}] disconnected, re-starting...", symbol);
            // this->start();

            MongoDB::instance()
                .set_db_and_collection("websocket_monitoring", "TradeDataWebsocket")
                .insert_one(Json{
                    {"event", "DISCONNECTED"},
                    {"symbol", m_symbol},
                    {"timestamp", Utils::get_time_now_in_string_HMS_DMY()}
                }
            );

            co_return;
        },
        // on_close
        [symbol = m_symbol]() -> Task<void>
        {
            spdlog::debug("BinanceTradeData [{}] closed", symbol);
            co_return;
        }
    );
}