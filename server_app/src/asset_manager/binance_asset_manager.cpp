#include <asset_manager/binance_asset_manager.h>
#include <strategy_engine/scanning_strategies_manager.h>

#include <exchanges/exchange_gateway.h>

// #include <json/json.h>

#include <app_constants.h>
#include <utils.h>
#include <app_utils.h>

using namespace std;

void BinanceAssetManager::init()
{
    fetch_positions();

    m_binance_spot_callback_id = 0;
    m_binance_futures_callback_id = 0;

    subscribe_data();
}

void BinanceAssetManager::subscribe_data()
{
    // subscribe trade data
    auto events_updated_callback = [this](Json& data)
        {
            process_events_updated(data);
        };

    if (m_binance_spot_callback_id == 0)
        m_binance_spot_callback_id =
            ExchangeGateWay::instance().subscribe_user_feed(BINANCE_SPOT, events_updated_callback);

    if (m_binance_futures_callback_id == 0)
        m_binance_futures_callback_id =
            ExchangeGateWay::instance().subscribe_user_feed(BINANCE_FUTURES, events_updated_callback);
}

void BinanceAssetManager::unsubscribe_data()
{
    if (m_binance_spot_callback_id > 0)
        ExchangeGateWay::instance().unsubscribe_user_feed(BINANCE_SPOT, m_binance_spot_callback_id);

    if (m_binance_futures_callback_id > 0)
        ExchangeGateWay::instance().unsubscribe_user_feed(BINANCE_SPOT, m_binance_futures_callback_id);
}

void BinanceAssetManager::process_events_updated(Json& data)
{
    if (!data.has_field("event")) return;

    if ((string&&)data["event"] == "orderUpdate")
        save_spot_and_future_trade(data);
    else if ((string&&)data["event"] == "positionUpdate")
        save_position(data);
}

void BinanceAssetManager::save_spot_and_future_trade(Json& trade)
{
    unique_lock lock(m_save_db_mutex);
    ADD_LOG("save_spot_and_future_trade 1");

    if ((string&&)trade["status"] != "PARTIALLY_FILLED" &&
        (string&&)trade["status"] != "FILLED")
        return;

    // get exchange info to get quoteAsset
    Json m_symbol_info;
    if ((string&&)trade["exchange"] == BINANCE_SPOT_ABBREVIATION_NAME)
        m_symbol_info = ExchangeGateWay::instance().get_exchange_info(BINANCE_SPOT, trade["symbol"]);
    else
        m_symbol_info = ExchangeGateWay::instance().get_exchange_info(BINANCE_FUTURES, trade["symbol"]);

    ADD_LOG("save_spot_and_future_trade 2");

    Json trade_report;
    trade_report["Date"] = trade["transactTime"];
    trade_report["Asset"] = trade["symbol"];
    trade_report["Market"] = trade["exchange"];
    trade_report["Asset-Currency"] = m_symbol_info["quoteAsset"];
    trade_report["Price"] = trade["lastExecutedPrice"];
    trade_report["Fee"] = trade["commissionAmount"];
    trade_report["Fee-Currency"] = trade["commissionAsset"];
    trade_report["Order ID"] = trade["orderId"];

    long double quantity = stold((string&&)trade["lastExecutedQuantity"]);
    if ((string&&)trade["side"] == "SELL")
        quantity = -quantity;
    trade_report["Position"] = TO_STRING(quantity, 8);

    ADD_LOG("trade_report 1: " << trade_report.get_string_value());

    // attach trade data info
    string client_order_id = (string&&)trade["clientOrderId"];
    vector<string> str_arr = Utils::instance().split_string(client_order_id, "_");

    // check trade from my app
    if (str_arr.size() == 2 && AppUtils::instance().is_long_number(str_arr[0]) &&
        AppUtils::instance().is_long_number(str_arr[1]))
    {
        long strategy_id = stol(str_arr[0]);
        ScanningStrategiesManager::instance().attach_trade_data_info(strategy_id, trade_report);
    }

    ADD_LOG("trade_report 2: " << trade_report.get_string_value());

    // save to db
    save_trade_report_to_db(trade_report);

    // send to client
    send_trade_to_client(trade_report);
}

void BinanceAssetManager::save_trade_report_to_db(Json& trade_report)
{
    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(TRADE_HISTORY_DB, BINANCE_TRADE_HISTORY);
    query.insert_one(trade_report);
}

void BinanceAssetManager::send_trade_to_client(Json& trade)
{
    WebSocketServerType::instance().send_data_through_channel(
        CHANNEL_TRADE_HISTORY,
        "root",
        trade
    );
}

void BinanceAssetManager::send_position_to_client(Json& position)
{
    WebSocketServerType::instance().send_data_through_channel(
        CHANNEL_POSITION_UPDATE,
        "root",
        position
    );
}

Json BinanceAssetManager::get_trade_result(const long date_from, const long date_to)
{
    unique_lock lock(m_save_db_mutex);

    bsoncxx::v_noabi::document::view_or_value filter = document{} <<
        "Date" << open_document <<
            "$gt" << date_from <<
            "$lte" << date_to <<
        close_document << finalize;

    Json ret;
    ret = MongoDB::instance()
        .set_db_and_collection(TRADE_HISTORY_DB, BINANCE_TRADE_HISTORY)
        .find_many(filter);

    ret.for_each([](Json& data)
    {
        data.remove_field("_id");
        if (!data.has_field("Market"))
            data["Market"] = "UNKNOWN";
    });

    return ret;
}

void BinanceAssetManager::query_blvt_record(string side, const long orderId, const long strategy_id)
{
    unique_lock lock(m_save_db_mutex);

    Json query_json = {
        {"side", side},
        {"orderId", orderId}
    };

    Json response = ExchangeGateWay::instance().get_order(BINANCE_BLVT, query_json);
    if (response["error"] == false)
    {
        Json trade_report;
        trade_report["Date"] = response["data"]["timestamp"];
        trade_report["Asset"] = (string&&)response["data"]["symbol"]+"USDT";
        trade_report["Market"] = response["data"]["exchange"];
        trade_report["Asset-Currency"] = "USDT";
        trade_report["Price"] = response["data"]["price"];
        trade_report["Fee"] = response["data"]["fee"];
        trade_report["Fee-Currency"] = "USDT";
        trade_report["Order ID"] = response["data"]["orderId"];

        long double quantity = stold((string&&)response["data"]["quantity"]);
        if ((string&&)response["data"]["side"] == "SELL")
            quantity = -quantity;
        trade_report["Position"] = TO_STRING(quantity, 8);

        // attach trade data info
        ScanningStrategiesManager::instance().attach_trade_data_info(strategy_id, trade_report);

        ADD_LOG("trade_report: " << trade_report.get_string_value());

        // save to db
        save_trade_report_to_db(trade_report);

        // send to client
        send_trade_to_client(trade_report);
    }
}

Json BinanceAssetManager::get_current_assets()
{
    Json ret = Json::create_array();
    for (auto i : m_assets_map)
    {
        ret.push_back(i.second);
    }

    return ret;
}

void BinanceAssetManager::fetch_positions()
{
    // fetch spot
    Json spot_asset = ExchangeGateWay::instance().get_account_info(BINANCE_SPOT);
    if (spot_asset["error"] == false)
        save_position(spot_asset["data"], false);

    // fetch futures
    Json futures_asset = ExchangeGateWay::instance().get_account_info(BINANCE_FUTURES);
    if (futures_asset["error"] == false)
        save_position(futures_asset["data"], false);
}

void BinanceAssetManager::save_position(Json& data, bool forward_to_client)
{
    unique_lock lock(m_save_db_mutex);
    ADD_LOG("save_position: " << data.get_string_value());

    string market = (string&&)data["exchange"];
    Json positions = data["positionList"];
    positions.for_each([this, market, forward_to_client] (Json& json)
    {
        Json position;
        position["Symbol"] = json["symbol"];
        position["Market"] = market;
        position["Price"] = json["price"];
        position["Quantity"] = json["quantity"];

        // send to client
        if (forward_to_client)
        {
            this->send_position_to_client(position);
        }

        // update data
        string key = market + "#" + (string&&)position["Symbol"];
        if (IS_EQUAL((long double)0.0, stold((string&&)position["Quantity"])))
        {
            if (this->m_assets_map.find(key) != this->m_assets_map.end())
                this->m_assets_map.erase(key);
        }
        else
            this->m_assets_map[key] = position;
    });
}

bool BinanceAssetManager::check_asset_available(Market market, Json& query_json)
{
    if (market == BINANCE_SPOT)
    {
        if ((string&&)query_json["side"] == "SELL")
        {
            string key = BINANCE_SPOT_ABBREVIATION_NAME + "#" + (string&&)query_json["symbol"];
            if (m_assets_map.find(key) != this->m_assets_map.end())
                if (stold((string&&)query_json["quantity"]) < stold((string&&)m_assets_map[key]["Quantity"]))
                {
                    ADD_LOG("Spot asset is not available for SELL: " << (string&&)query_json["symbol"]);
                    return false;
                }
        }
    }

    return true;
}
