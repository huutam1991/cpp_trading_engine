//#include <json/json.h>
//#include <app_constants.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance_depth.h>

DataFeedBinanceDepth::DataFeedBinanceDepth(const std::string& symbol):
    m_symbol(symbol)
{
}

DataFeedBinanceDepth::~DataFeedBinanceDepth()
{
    ADD_LOG("~DataFeedBinanceDepth, " << m_symbol);
}

void DataFeedBinanceDepth::init()
{
    //check_ws_url_base_on_back_testing();
    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, m_binance_ws_port, "/ws");

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket depth connected");

        // Subcribe for depth
        m_id = DataFeedBinance::get_stream_id_count();
        std::string lower_case_symbol = m_symbol;
        STRING_LOWER_CASE(lower_case_symbol);

        Json params;
        params[0] = lower_case_symbol + "@depth5@100ms";

        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = m_id;

        ADD_LOG("subcribe = " << subcribe);

        ws.write(subcribe.get_string_value());
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        Json depth = Json();
        if (this->standardize_data(buffer, depth))
        {
            // ADD_LOG("Stream depth: " << this->m_symbol);
            if (m_on_callback != nullptr)
            {
                m_on_callback(m_symbol, depth);
            }
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("Stream depth close: " << this->m_symbol);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start DataFeedBinanceDepth");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceDepth::set_call_back(std::function<void(const std::string& symbol, Json& payload)> call_back)
{
    m_on_callback = call_back;
}

bool DataFeedBinanceDepth::standardize_data(const std::string& data, Json& depth)
{
    Json order_book = Json::parse(data);
    if (order_book.has_field("asks") && order_book.has_field("bids"))
    {
        // symbol
        depth["s"] = BINANCE_SPOT_ABBREVIATION_NAME + "#" + m_symbol;
        // event name
        depth["e"] = "depthUpdate";

        // update asks
        Json A = Json::create_array();
        Json asks = order_book["asks"];
        asks.for_each([&A](Json& data)
        {
            Json j = Json::create_array();
            j.push_back(std::stold((std::string&&)data[0]));
            j.push_back(std::stold((std::string&&)data[1]));
            A.push_back(j);
        });
        depth["asks"] = A;

        // update bids
        Json B = Json::create_array();
        Json bids = order_book["bids"];
        bids.for_each([&B](Json& data)
        {
            Json j = Json::create_array();
            j.push_back(std::stold((std::string&&)data[0]));
            j.push_back(std::stold((std::string&&)data[1]));
            B.push_back(j);
        });
        depth["bids"] = B;
        return true;
    }
    return false;
}