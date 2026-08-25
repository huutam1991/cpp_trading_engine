#include "trade_data.h"
#include <app_constants.h>
#include <json/json.h>
#include <utils/utils.h>
#include <mongo_db/mongo_db.h>
#include <enum_reflect/enum_reflect.h>

#include <strategy/strategy_manager.h>

BinanceTradeData::BinanceTradeData(const Instrument* instrument, EpollBase* event_base)
    : m_instrument{instrument}, m_event_base{event_base}
{
    m_symbol = instrument->exchange_symbol.to_string();
    STRING_LOWER_CASE(m_symbol);
    start();
}

void BinanceTradeData::start()
{
    // Check to close current websocket if any
    if (m_websocket != nullptr)
    {
        m_websocket = nullptr;
    }

    std::string ws_path = "/market/ws/" + m_symbol + "@aggTrade";

    std::string name = m_symbol + "_trade_data_ws";
    std::string url = BINANCE_FUTURES_WS_URL;
    int port = std::stoi(BINANCE_FUTURES_WS_PORT);

    if (m_instrument->exchange_id == ExchangeId::BINANCE_TESTNET)
    {
        url = BINANCE_TESTNET_FUTURES_WS_URL;
        port = std::stoi(BINANCE_TESTNET_FUTURES_WS_PORT);
    }

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_event_base, url, port, ws_path,
        // on_connect
        [this, ws_path]() -> Task<void>
        {
            spdlog::info("BinanceTradeData [{}] is connected, exchange: {}", ws_path, enum_reflect::enum_name(m_instrument->exchange_id));

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