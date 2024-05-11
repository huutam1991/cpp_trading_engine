#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_info.h>

DataFeedBinanceBLVTInfo::DataFeedBinanceBLVTInfo(const string& symbol):
    m_symbol(symbol)
{
}

DataFeedBinanceBLVTInfo::~DataFeedBinanceBLVTInfo()
{
    ADD_LOG("~DataFeedBinanceBLVTInfo: " << m_symbol);
}

void DataFeedBinanceBLVTInfo::init()
{
    //check_ws_url_base_on_back_testing();
    m_websocket = make_shared<WebsocketClient>(m_binance_ws_url, 
                                                m_binance_ws_port, 
                                                "/lvt-p/ws");

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket BLVT connected");

        // Subcribe for BLVT
        m_id = DataFeedBinanceBLVT::get_stream_id_count();

        Json params;
        params[0] = m_symbol + "@tokenNav";

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
        ADD_LOG("Stream Binance websocket BLVT close: " << this->m_symbol);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start DataFeedBinanceBLVTInfo");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceBLVTInfo::set_call_back(function<void(const string& symbol, Json& payload)> call_back)
{
    m_on_callback = call_back;
}

bool DataFeedBinanceBLVTInfo::standardize_data(const string& buffer, Json& data)
{
    Json blvt_info = Json::parse(buffer);
    if (blvt_info.has_field("e") && (string&&)blvt_info["e"] == "nav")
    {
        // symbol
        // data["s"] = BINANCE_SPOT_ABBREVIATION_NAME + "#" + m_symbol;
        data["s"] = m_symbol;
        // event name
        data["e"] = "tokenNav";

        // Token issued
        data["m"] = (long double)blvt_info["m"];

        // BLVT NAV
        data["n"] = (long double)blvt_info["n"];

        // Real leverage
        data["l"] = (long double)blvt_info["l"];
        
        // Target leverage        
        data["t"] = (long double)blvt_info["t"];

        // get basket
        Json basket = blvt_info["b"][0];
        data["bn"] = (long double)basket["n"];

        return true;
    }

    return false;
}