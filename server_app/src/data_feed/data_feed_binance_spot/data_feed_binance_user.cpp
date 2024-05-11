#include <timer.h>
#include <external_request/external_request_ssl.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance_user.h>

DataFeedBinanceUser::DataFeedBinanceUser(const string api_key, const string api_secret)
{
    m_api_key    = api_key;
    m_api_secret = api_secret;
}

DataFeedBinanceUser::~DataFeedBinanceUser()
{
    // Delete current schedule task to re-active m_listen_key
    this->del_timer_keep_alive_listen_key();
}

/*void DataFeedBinanceUser::add_new_binance_user_data_stream(const StorageSourceBinance& storage_source)
{
    std::string storage_source_id = storage_source.get_id();

    // Remove the DataFeedBinanceUser has the same storage_source_id
    if (m_data_user_stream_list.find(storage_source_id) != m_data_user_stream_list.end())
    {
        m_data_user_stream_list.erase(storage_source_id);
    }

    // Insert DataFeedBinanceUser, then start
    m_data_user_stream_list.insert(std::make_pair(storage_source_id, DataFeedBinanceUser(storage_source)));
    m_data_user_stream_list[storage_source_id].start();
}*/

void DataFeedBinanceUser::init()
{
    m_listen_key = get_listen_key();

    // Set period time to re-active m_listen_key at every 30 minutes (1800 seconds)
    add_timer_keep_alive_listen_key(1800000);

    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, m_binance_ws_port, "/ws/" + m_listen_key);
    m_websocket->set_use_valid_data(true);

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("BinanceSpotUser websocket connected, source: [" + m_db_name + "]");
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        // join message here
        Json json = Json::parse(buffer);
        ADD_LOG("BinanceSpotUser on_message: " << json.get_string_value());

        if (json.has_field("e") == true)
        {
            std::string event_type = json["e"];
            
            // Account update
            if (event_type == "outboundAccountPosition")
            {
                Json std_account_report;
                this->standardize_account_report(json, std_account_report);

                // Send to all subscribers
                for(auto cb : m_subscribed_list)
                {
                    cb.second(std_account_report);
                }
            }
            // Balance update
            else if (event_type == "balanceUpdate")
            {
            }
            // Order update
            else if (event_type == "executionReport")
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
        ADD_LOG("BinanceSpotUser on_close, close_code = " << close_code);

        // Unexpected close, need to re-start
        if (close_code == websocket::close_code::internal_error)
        {
            // Delete current schedule task to re-active m_listen_key
            this->del_timer_keep_alive_listen_key();

            // Re-start
            ADD_LOG("BinanceSpot - re-starting");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceUser::standardize_account_report(Json& report, Json& std_report)
{
    std_report["event"] = "positionUpdate";
    std_report["exchange"] = BINANCE_SPOT_ABBREVIATION_NAME;
    std_report["time"] = report["E"];

    Json balances = report["B"];
    Json balance_list = Json::create_array();
    balances.for_each([&balance_list](Json& balance)
    {
        Json info;
        info["symbol"] = balance["a"];
        info["price"] = 0;
        info["quantity"] = balance["f"];
        balance_list.push_back(info);
    });

    std_report["positionList"] = balance_list;

    ADD_LOG("standardize_account_report: " << std_report.get_string_value());
}

void DataFeedBinanceUser::standardize_execution_report(Json& json, Json& res)
{
    res["event"] = "orderUpdate";
    res["exchange"] = BINANCE_SPOT_ABBREVIATION_NAME;
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

Json DataFeedBinanceUser::clone_standard_order_object(Json& json)
{
    Json res;
    res["symbol"] = json["s"];
    res["clientOrderId"] = json["c"];
    res["cummulativeQuoteQty"] = json["Z"];
    res["orderId"] = json["i"];
    res["orderListId"] = json["g"];
    res["transactTime"] = json["T"];
    res["price"] = (json["o"] == "MARKET") ? json["L"] : json["p"];
    res["origQty"] = json["q"];
    res["executedQty"] = json["z"];
    res["last_executed_quantity"] = res["executedQty"];
    res["type"] = json["o"];
    res["status"] = json["X"];
    res["timeInForce"] = json["f"];
    res["side"] = json["S"];

    return res;
}

std::string DataFeedBinanceUser::get_listen_key()
{
    ExternalRequestSsl binance_request(m_url, m_port, "/api/v3/userDataStream", RequestMethod::POST);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    std::string res = binance_request.send_request("");
    Json data = Json::parse(res.substr(res.find("|")+1));
    return data["listenKey"];
}

void DataFeedBinanceUser::add_timer_keep_alive_listen_key(size_t period)
{
    m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    {
        ExternalRequestSsl binance_request(m_url, "443", "/api/v3/userDataStream?listenKey=" + m_listen_key, RequestMethod::PUT);
        binance_request.add_header("X-MBX-APIKEY", m_api_key);

        ADD_LOG("Source [" + m_db_name + "]: re-active m_listen_key = " << m_listen_key);

        std::string res = binance_request.send_request("");
        Json data = Json::parse(res.substr(res.find("|")+1));
        ADD_LOG("Source [" + m_db_name + "]: re-active m_listen_key = " << m_listen_key << " - END ");
        ADD_LOG("re-active data: " << data.get_string_value());
    },
    period);
}

void DataFeedBinanceUser::del_timer_keep_alive_listen_key()
{
    if (m_schedule_task_id != 0)
    {
        Timer::instance().delete_schedule_task(this->m_schedule_task_id);
    }
}

size_t DataFeedBinanceUser::add_call_back(std::function<void(Json& payload)> call_back)
{
    std::unique_lock lock(m_bsuser_mutex);
    m_callback_id++;
    m_subscribed_list.insert({m_callback_id, call_back});

    ADD_LOG("add_call_back, " << m_callback_id);
    return m_callback_id;
}

void DataFeedBinanceUser::remove_call_back(size_t callback_id)
{
    std::unique_lock lock(m_bsuser_mutex);
    m_subscribed_list.erase(callback_id);
}

std::map<size_t, std::function<void(Json& payload)>> DataFeedBinanceUser::m_subscribed_list;
//std::unordered_map<std::string,DataFeedBinanceUser> DataFeedBinanceUser::m_data_user_stream_list;