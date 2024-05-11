//#include <json/json.h>
//#include <app_constants.h>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures_agg_trade.h>

DataFeedBinanceFuturesAggTrade::DataFeedBinanceFuturesAggTrade(const std::string& symbol):
    m_symbol(symbol)
{
}

DataFeedBinanceFuturesAggTrade::~DataFeedBinanceFuturesAggTrade()
{
    ADD_LOG("~DataFeedBinanceFuturesAggTrade, " << m_symbol);
}

void DataFeedBinanceFuturesAggTrade::init()
{
    //check_ws_url_base_on_back_testing();
    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, m_binance_ws_port, "/ws");

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket aggTrade connected");

        // Subcribe for depth
        m_id = DataFeedBinance::get_stream_id_count();
        std::string lower_case_symbol = m_symbol;
        STRING_LOWER_CASE(lower_case_symbol);

        Json params;
        params[0] = lower_case_symbol + "@aggTrade";

        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = m_id;

        ADD_LOG("subcribe = " << subcribe);

        ws.write(subcribe.get_string_value());
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        Json aggTrade = Json();
        if (this->standardize_data(buffer, aggTrade))
        {
            // ADD_LOG("Stream depth: " << this->m_symbol);
            if (m_on_callback != nullptr)
            {
                m_on_callback(m_symbol, aggTrade);
            }
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("Stream agg_trade close");
        ADD_LOG("close_code = " << close_code);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start DataFeedBinanceFuturesAggTrade");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceFuturesAggTrade::set_call_back(std::function<void(const std::string& symbol, Json& payload)> call_back)
{
    m_on_callback = call_back;
}

bool DataFeedBinanceFuturesAggTrade::standardize_data(const string& buffer, Json& data)
{
    Json aggTrade = Json::parse(buffer);
    if (aggTrade.has_field("e") && (string&&)aggTrade["e"] == "aggTrade")
    {
        // symbol
        data["s"] = BINANCE_FUTURES_ABBREVIATION_NAME + "#" + m_symbol;
        // event name
        data["e"] = "aggTrade";

        // last price
        data["p"] = std::stold((std::string&&)aggTrade["p"]);
        // quantity
        data["q"] = std::stold((std::string&&)aggTrade["q"]);

        // ADD_LOG("Stream aggTrade: " << data.get_string_value());
        return true;
    }
    return false;
}