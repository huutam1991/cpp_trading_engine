#pragma once

#include <string>

#include <websocket/websocket_server_ssl.h>
#include <websocket/websocket_server.h>

// App pool
#define NUMBER_OF_APP_THREADS 1

// Token
#define TOKEN_ISSUER "TamNguyen"
#define TOKEN_EXPRIED_TIME 168
#define TOKEN_SECRET_KEY "CPP_TRADING_ENGINE_ON_BINANCE_22661"

// Channel name
#define CHANNEL_ORDER_STATUS "order_status"
#define CHANNEL_PROFIT "profit"
#define CHANNEL_BALANCE "balance"
#define CHANNEL_AUTO_TRADE "auto_trade"
#define CHANNEL_SCANNING_MARKET "scanning_market"
#define CHANNEL_SCANNING_MARKET_NOTIFICATION "scanning_market_notification"
#define CHANNEL_TRADE_HISTORY "trade_history"
#define CHANNEL_POSITION_UPDATE "position_update"

// using WebSocketServerType = WebsocketServer<session,listener>;
using WebSocketServerType = WebsocketServer<session_ssl,listener_ssl>;

enum class SourceType
{
    NONE,
    BINANCE_TESTNET,
    BINANCE_SIMULATOR,
    BINANCE_SPOT,
    BINANCE_BLVT,
    BINANCE_FUTURES,
    COINBASE_TESTNET,
    COINBASE_REALNET,
    FTX_TESTNET,
    FTX_REALNET
};

// Scanning market config DB name
#define SCANNING_MARKET_CONFIG "scanning_market_config"

// Scanning market config DB name
#define SCANNING_BLVT_CONFIG "scanning_blvt_config"

// Trade history DB name
#define TRADE_HISTORY_DB "trade_history"

// Binance trade history collection
#define BINANCE_TRADE_HISTORY "binance"

// Execution report collection name
#define EXECUTION_REPORT "execution_report"

// Price ticker collection name
#define PRICE_TICKER "price_ticker"

// Trade error collection name
#define TRADE_ERROR "trade_error"

// Trading result collection name
#define TRADING_RESULT "trading_result"

// App info DB name
#define APP_INFO_DB_NAME "app_info"

// Order DB name
#define ORDER_DB_NAME "order"

// Strategy DB name
#define STRATEGY_DB_NAME "strategy"
#define STRATEGY_PRICE_ARBITRAGE_DB_NAME "strategy_price_arbitrage"

// None Source
#define NONE_DB_SOURCE_NAME "none_source"

// Binance common
#define BINANCE_COMMON "binance_common"

// Binance testnet
#define BINANCE_TESTNET_SPOT_DB_SOURCE_NAME "binance_test"
#define BINANCE_TESTNET_SPOT_URL "testnet.binance.vision"
#define BINANCE_TESTNET_SPOT_PORT "443"
#define BINANCE_TESTNET_SPOT_WS_URL "testnet.binance.vision"
#define BINANCE_TESTNET_SPOT_WS_PORT "443"

// Binance Spot
#define BINANCE_SPOT_DB_SOURCE_NAME "binance_spot"
#define BINANCE_SPOT_URL "api.binance.com"
#define BINANCE_SPOT_PORT "443"
#define BINANCE_SPOT_WS_URL "stream.binance.com"
#define BINANCE_SPOT_WS_PORT "9443"

// Binance Testnet Fututes
#define BINANCE_TESTNET_FUTURES_DB_SOURCE_NAME "binance_test_futures"
#define BINANCE_TESTNET_FUTURES_URL "testnet.binancefuture.com"
#define BINANCE_TESTNET_FUTURES_PORT "443"
#define BINANCE_TESTNET_FUTURES_WS_URL "fstream.binancefuture.com"
#define BINANCE_TESTNET_FUTURES_WS_PORT "443"

// Binance Fututes
#define BINANCE_FUTURES_DB_SOURCE_NAME "binance_futures"
#define BINANCE_FUTURES_URL "fapi.binance.com"
#define BINANCE_FUTURES_PORT "443"
#define BINANCE_FUTURES_WS_URL "fstream.binance.com"
#define BINANCE_FUTURES_WS_PORT "443"

// Binance BLVT
#define BINANCE_BLVT_DB_SOURCE_NAME "binance_blvt"
#define BINANCE_BLVT_URL "api.binance.com"
#define BINANCE_BLVT_PORT "443"
#define BINANCE_BLVT_WS_URL "nbstream.binance.com"
#define BINANCE_BLVT_WS_PORT "443"

// Binance simulator
#define BINANCE_SIMULATOR_DB_SOURCE_NAME "binance_simulator"
#define BINANCE_SIMULATOR_URL "127.0.0.1"
#define BINANCE_SIMULATOR_PORT "4443"
#define BINANCE_SIMULATOR_WS_URL "127.0.0.1"
#define BINANCE_SIMULATOR_WS_PORT "5443"

// Coinbase testnet
#define COINBASE_TESTNET_DB_SOURCE_NAME "coinbase_test"
#define COINBASE_TESTNET_URL "api-public.sandbox.exchange.coinbase.com"
#define COINBASE_TESTNET_PORT "443"
#define COINBASE_TESTNET_WS_URL "ws-feed.exchange.coinbase.com" // Need check
#define COINBASE_TESTNET_WS_PORT "443"

// Coinbase realnet
#define COINBASE_REALNET_DB_SOURCE_NAME "coinbase_real"
#define COINBASE_REALNET_URL "api.exchange.coinbase.com"
#define COINBASE_REALNET_PORT "443"
#define COINBASE_REALNET_WS_URL "ws-feed.exchange.coinbase.com" // Need check
#define COINBASE_REALNET_WS_PORT "9443"

// Coinbase advance realnet
#define COINBASE_ADVANCE_REALNET_DB_SOURCE_NAME "coinbase_advance_real"
#define COINBASE_ADVANCE_REALNET_URL "api.coinbase.com"
#define COINBASE_ADVANCE_REALNET_PORT "443"
#define COINBASE_ADVANCE_REALNET_WS_URL "advanced-trade-ws.coinbase.com"
#define COINBASE_ADVANCE_REALNET_WS_PORT "443"

// FTX testnet
#define FTX_TESTNET_DB_SOURCE_NAME "ftx_test"
#define FTX_TESTNET_URL "testnet.ftx.vision"
#define FTX_TESTNET_PORT "443"
#define FTX_TESTNET_WS_URL "testnet.ftx.vision"
#define FTX_TESTNET_WS_PORT "443"

// FTX realnet
#define FTX_REALNET_DB_SOURCE_NAME "ftx_real"
#define FTX_REALNET_URL "api.ftx.com"
#define FTX_REALNET_PORT "443"
#define FTX_REALNET_WS_URL "stream.ftx.com"
#define FTX_REALNET_WS_PORT "9443"

// Strategy name
#define BLVT_TRADING_STRATEGY_UP_NAME               "TLToken_Up"
#define BLVT_TRADING_STRATEGY_DOWN_NAME             "TLToken_Down"

#define MARKET_MAKING_TRADING_STRATEGY_UP_NAME      "T_MM_Up"
#define MARKET_MAKING_TRADING_STRATEGY_DOWN_NAME    "T_MM_Down"

#define MOMENTUM_HEDGING_STRATEGY_NAME              "HMomentum"
#define FUTURES_HEDGING_STRATEGY_NAME               "HFutures"

// exchange abbreviation names
inline static const std::string BINANCE_SPOT_ABBREVIATION_NAME =    "BIN_SPOT";
inline static const std::string BINANCE_FUTURES_ABBREVIATION_NAME = "BIN_FUT";
inline static const std::string BINANCE_NAV_ABBREVIATION_NAME =     "BIN_NAV";

enum Market
{
    BINANCE_SPOT			= 0,
    BINANCE_FUTURES 		= 1,
    BINANCE_BLVT            = 2
};

enum MDSubscribeType
{
    DEPTH                   = 0,
    BOOK_TICKER             = 1,
    AGG_TRADE               = 2,
    MARKET_INFO             = 3,
    K_LINE                  = 4
};

enum NotificationState
{
    NOTIFICATION_STATE_BASKETS_CHANGE           = 1,
    NOTIFICATION_STATE_TOKEN_ISSUED_CHANGE      = 2,
    NOTIFICATION_STATE_TRADE_ERROR              = 3
};

enum IOCId
{
    TIMER,
    MARKET_DATA,
    ORDER_ENTRY, 
};

enum EventBaseID
{
    APP,
    ORDER,                   // OrderManager
    DB_HELPER,               // Help to data to MongoDB
    GATEWAY,                 // Gateway
    STRATEGY,
    PRICE_ARBITRAGE_STRATEGY // Strategy - Price Arbitrage
};
