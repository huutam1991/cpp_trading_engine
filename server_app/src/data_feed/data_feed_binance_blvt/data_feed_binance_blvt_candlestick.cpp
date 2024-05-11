#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_candlestick.h>

DataFeedBinanceBLVTCandlestick::DataFeedBinanceBLVTCandlestick(const string& symbol):
    m_symbol(symbol)
{
}

DataFeedBinanceBLVTCandlestick::~DataFeedBinanceBLVTCandlestick()
{
    ADD_LOG("~DataFeedBinanceBLVTCandlestick: " << m_symbol);
}

void DataFeedBinanceBLVTCandlestick::init()
{
    //check_ws_url_base_on_back_testing();
    m_websocket = make_shared<WebsocketClient>(m_binance_ws_url, 
                                                m_binance_ws_port, 
                                                "/lvt-p/ws");

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket BLVT Candlestick connected");

        // Subcribe for BLVT
        m_id = DataFeedBinanceBLVT::get_stream_id_count();

        Json params;
        params[0] = m_symbol + "@nav_kline_1m";

        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = m_id;

        ADD_LOG("subcribe = " << subcribe);

        ws.write(subcribe.get_string_value());
    });

    m_websocket->on_message([this](const string& buffer, WebsocketClientHandle& ws)
    {
        Json blvt_info = Json();
        if (this->standardize_data(buffer, blvt_info))
        {
            if (m_on_callback != nullptr)
            {
                m_on_callback(m_symbol, blvt_info);
            }
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("Stream Binance websocket BLVT Candlestick close: " << this->m_symbol);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start DataFeedBinanceBLVTCandlestick");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceBLVTCandlestick::set_call_back(function<void(const string& symbol, Json& payload)> call_back)
{
    m_on_callback = call_back;
}

bool DataFeedBinanceBLVTCandlestick::standardize_data(const string& buffer, Json& data)
{
    Json blvt_info = Json::parse(buffer);
    if (blvt_info.has_field("e") && (string&&)blvt_info["e"] == "kline")
    {
        // symbol
        // data["s"] = BINANCE_SPOT_ABBREVIATION_NAME + "#" + m_symbol;
        data["s"] = BINANCE_NAV_ABBREVIATION_NAME + "#" + m_symbol;
        // event name
        data["e"] = "kline";
        // get kline info
        Json kline = blvt_info["k"];
        // update prices
        data["o"] = stold((string&&)kline["o"]);
        data["c"] = stold((string&&)kline["c"]);
        data["h"] = stold((string&&)kline["h"]);
        data["l"] = stold((string&&)kline["l"]);
        
        return true;
    }

    return false;
}