//#include <json/json.h>
//#include <app_constants.h>
#include <price_manager/price_manager.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance.h>
//#include <back_testing/back_testing.h>

std::mutex DataFeedBinance::m_df_bs_mutex;

DataFeedBinance::DataFeedBinance() : DataFeed(BINANCE_COMMON)
{
    m_url = BINANCE_SPOT_URL;
    m_port = BINANCE_SPOT_PORT;
    m_binance_ws_url  = BINANCE_SPOT_WS_URL;
    m_binance_ws_port = BINANCE_SPOT_WS_PORT;
}

/*void DataFeedBinance::check_ws_url_base_on_back_testing()
{
    m_url = BINANCE_SPOT_URL;
    m_port = BINANCE_SPOT_PORT;
    m_binance_ws_url  = BINANCE_SPOT_WS_URL;
    m_binance_ws_port = BINANCE_SPOT_WS_PORT;

    if (BackTesting::instance().is_back_testing_mode() == true)
    {
        m_binance_ws_url  = BINANCE_SIMULATOR_WS_URL;
        m_binance_ws_port = BINANCE_SIMULATOR_WS_PORT;

        // Check environment variable
        if (const char* uri_env = std::getenv("BINANCE_SIMULATOR_URL"))
        {
            m_binance_ws_url = uri_env;
        }
    }
}*/

size_t DataFeedBinance::get_stream_id_count()
{
    static int stream_id_count = 0;
    std::unique_lock lock(m_df_bs_mutex);

    return ++stream_id_count;
}

void DataFeedBinance::init()
{
    //check_ws_url_base_on_back_testing();
    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, m_binance_ws_port, "/ws");
    m_websocket->set_use_valid_data(true);

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket connected");

        // Subcribe for miniTicker
        m_id = DataFeedBinance::get_stream_id_count();

        Json params;
        params[0] = "!miniTicker@arr";

        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = m_id;

        ws.write(subcribe.get_string_value());
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        static int counter = 0;
        Json json = Json::parse(buffer);

        if (json.has_field("result") == false || json["result"].get_string_value() != "null")
        {
            // Is mini ticker
            if (json.is_array())
            {
                json.for_each([this](Json& data)
                {
                    // this->update_db_current_price(data);
                    PriceManager::instance().set_price(data);
                });

                // Add counter here as mini ticker is updated per second
                // ADD_LOG("Binance: " << counter++);
            }
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("Stream current_price close, close_code = " << close_code);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start DataFeedBinance");
            this->start();
        }
    });

    m_websocket->run();
}