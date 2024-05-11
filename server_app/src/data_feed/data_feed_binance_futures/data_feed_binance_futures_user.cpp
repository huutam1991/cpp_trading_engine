#include <timer.h>
#include <external_request/external_request_ssl.h>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures_user.h>

DataFeedBinanceFuturesUser::DataFeedBinanceFuturesUser(const string api_key, const string api_secret)
{
    m_api_key    = api_key;
    m_api_secret = api_secret;
}

DataFeedBinanceFuturesUser::~DataFeedBinanceFuturesUser()
{
    // Delete current schedule task to re-active m_listen_key
    this->del_timer_keep_alive_listen_key();
}

/*void DataFeedBinanceFuturesUser::add_new_binance_user_data_stream(const StorageSourceBinanceFutures& storage_source)
{
    std::string storage_source_id = storage_source.get_id();

    // Remove the DataFeedBinanceFuturesUser has the same storage_source_id
    if (m_data_user_stream_list.find(storage_source_id) != m_data_user_stream_list.end())
    {
        m_data_user_stream_list.erase(storage_source_id);
    }

    // Insert DataFeedBinanceFuturesUser, then start
    m_data_user_stream_list.insert(std::make_pair(storage_source_id, DataFeedBinanceFuturesUser(storage_source)));
    m_data_user_stream_list[storage_source_id].start();
}*/

void DataFeedBinanceFuturesUser::init()
{
    m_listen_key = this->get_listen_key();

    // Set period time to re-active m_listen_key at every 30 minutes (1800 seconds)
    add_timer_keep_alive_listen_key(1800000);

    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, m_binance_ws_port, "/ws/" + m_listen_key);

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("BinanceFuturesUser websocket connected, source: [" + m_db_name + "]");
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        // ADD_LOG("BinanceFuturesUser OnMessage: " << buffer);
        Json json = Json::parse(buffer);
        ADD_LOG("BinanceFuturesUser on_message: " << json.get_string_value());

        if (json.has_field("e") == true)
        {
            std::string event_type = json["e"];

            // Account update
            if (event_type == "ACCOUNT_UPDATE")
            {
                Json std_account_report;
                this->standardize_account_report(json, std_account_report);

                // Send to all subscribers
                for(auto cb : m_subscribed_list)
                {
                    cb.second(std_account_report);
                }
            }
            // Order update
            else if (event_type == "ORDER_TRADE_UPDATE")
            {
                Json std_execution_report;
                this->standardize_execution_report(json, std_execution_report);

                // Send to all subscribers
                for(auto cb : m_subscribed_list)
                {
                    cb.second(std_execution_report);
                }
            }
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("BinanceFuturesUser on_close, close_code = " << close_code);

        // Unexpected close, need to re-start
        if (close_code == websocket::close_code::internal_error)
        {
            // Delete current schedule task to re-active m_listen_key
            this->del_timer_keep_alive_listen_key();

            // Re-start
            ADD_LOG("BinanceFutures - re-starting");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceFuturesUser::standardize_account_report(Json& report, Json& std_report)
{
    std_report["event"] = "positionUpdate";
    std_report["exchange"] = BINANCE_FUTURES_ABBREVIATION_NAME;
    std_report["time"] = report["E"];

    Json positions = report["a"]["P"];
    Json position_list = Json::create_array();
    positions.for_each([&position_list](Json& position)
    {
        Json info;
        info["symbol"] = position["s"];
        info["price"] = position["ep"];
        info["quantity"] = position["pa"];
        position_list.push_back(info);
    });

    std_report["positionList"] = position_list;

    ADD_LOG("standardize_account_report: " << std_report.get_string_value());
}

void DataFeedBinanceFuturesUser::standardize_execution_report(Json& jsons, Json& res)
{
    Json &json = jsons["o"];
    res["event"] = "orderUpdate";
    res["exchange"] = BINANCE_FUTURES_ABBREVIATION_NAME;
    res["time"] = json["E"];
    res["symbol"] = json["s"];
    res["status"] = json["X"];
    res["side"] = json["S"];
    res["orderType"] = json["o"];
    res["tradeId"] = json["t"];
    res["clientOrderId"] = json["c"];
    res["orderId"] = json["i"];
    res["transactTime"] = json["T"];
    res["lastExecutedPrice"] = json["L"];
    res["lastExecutedQuantity"] = json["l"];
    res["cumulativeFilledQuantity"] = json["z"];
    res["price"] = json["p"];
    res["origQty"] = json["q"];
    // process commission
    if (json.has_field("n"))
    {
        res["commissionAsset"] = json["N"];
        res["commissionAmount"] = json["n"];
    }
    else
    {
        res["commissionAsset"] = json["NaN"];
        res["commissionAmount"] = json["0"];
    }

    ADD_LOG("standardize_execution_report: " << res.get_string_value());
}

Json DataFeedBinanceFuturesUser::clone_standard_order_object(Json& jsons)
{
    Json res; Json &json = jsons["o"];
    res["exchange"] = BINANCE_FUTURES_ABBREVIATION_NAME;
    res["symbol"] = json["s"];
    res["clientOrderId"] = json["c"];
    res["cummulativeQuoteQty"] = 0;//json["Z"];
    res["orderId"] = json["i"];
    res["orderListId"] = 0;//json["g"];
    res["transactTime"] = json["T"];
    res["price"] = (json["o"] == "MARKET") ? json["ap"] : json["p"];
    res["origQty"] = json["q"];
    res["executedQty"] = json["z"];
    res["last_executed_quantity"] = res["executedQty"];
    res["type"] = json["o"];
    res["status"] = json["X"];
    res["timeInForce"] = json["f"];
    res["side"] = json["S"];

    return res;
}

std::string DataFeedBinanceFuturesUser::get_listen_key()
{
    ExternalRequestSsl binance_request(m_url, m_port, "/fapi/v1/listenKey", RequestMethod::POST);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    std::string res = binance_request.send_request("");
    Json data = Json::parse(res.substr(res.find("|")+1));
    return data["listenKey"];
}

void DataFeedBinanceFuturesUser::add_timer_keep_alive_listen_key(size_t period)
{
    m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    {
        ExternalRequestSsl binance_request(m_url, "443", "/fapi/v1/listenKey?listenKey=" + m_listen_key, RequestMethod::PUT);
        binance_request.add_header("X-MBX-APIKEY", m_api_key);

        ADD_LOG("User [], source [" + m_db_name + "]: re-active m_listen_key = " << m_listen_key);

        std::string res = binance_request.send_request("");
        Json data = Json::parse(res.substr(res.find("|")+1));
        ADD_LOG("User [], source [" + m_db_name + "]: re-active m_listen_key = " << m_listen_key << " - END");
        ADD_LOG("re-active data: " << data.get_string_value());
    },
    period);
}

void DataFeedBinanceFuturesUser::del_timer_keep_alive_listen_key()
{
    if (m_schedule_task_id != 0)
    {
        Timer::instance().delete_schedule_task(this->m_schedule_task_id);
    }
}

size_t DataFeedBinanceFuturesUser::add_call_back(std::function<void(Json& payload)> call_back)
{
    std::unique_lock lock(m_bfuser_mutex);
    m_callback_id++;
    m_subscribed_list.insert({m_callback_id, call_back});

    ADD_LOG("add_call_back, " << m_callback_id);
    return m_callback_id;
}

void DataFeedBinanceFuturesUser::remove_call_back(size_t callback_id)
{
    std::unique_lock lock(m_bfuser_mutex);
    m_subscribed_list.erase(callback_id);
}

std::map<size_t, std::function<void(Json& payload)>> DataFeedBinanceFuturesUser::m_subscribed_list;
//std::unordered_map<std::string, DataFeedBinanceFuturesUser> DataFeedBinanceFuturesUser::m_data_user_stream_list;