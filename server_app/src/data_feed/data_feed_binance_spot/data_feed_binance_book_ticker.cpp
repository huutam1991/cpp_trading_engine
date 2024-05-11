#include <data_feed/data_feed_binance_spot/data_feed_binance_book_ticker.h>
#include <utils.h>
//#include <json/json.h>

using namespace std;

DataFeedBinanceBookTicker::DataFeedBinanceBookTicker(const std::string& symbol):
    m_symbol(symbol)
{
}

DataFeedBinanceBookTicker::~DataFeedBinanceBookTicker()
{
    ADD_LOG("~DataFeedBinanceBookTicker, " << m_symbol);
}

void DataFeedBinanceBookTicker::init()
{
    //check_ws_url_base_on_back_testing();
    m_websocket = std::make_shared<WebsocketClient>(m_binance_ws_url, m_binance_ws_port, "/ws");

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket ticker connected");

        // Subcribe for bookTicker
        m_id = DataFeedBinance::get_stream_id_count();
        std::string lower_case_symbol = m_symbol;
        STRING_LOWER_CASE(lower_case_symbol);

        Json params;
        params[0] = lower_case_symbol + "@bookTicker";

        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = m_id;

        ADD_LOG("subcribe = " << subcribe);

        ws.write(subcribe.get_string_value());
    });

    m_websocket->on_message([this](const std::string& buffer, WebsocketClientHandle& ws)
    {
        Json ticker = Json();
        if (this->standardize_data(buffer, ticker))
        {
            // ADD_LOG("ticker = " << ticker);

            if (m_on_callback != nullptr)
            {
                m_on_callback(m_symbol, ticker);
            }
        }
    });

    m_websocket->on_close([this](websocket::close_code close_code)
    {
        ADD_LOG("Stream book ticker close: ");
        ADD_LOG("close_code = " << close_code);

        if (close_code == websocket::close_code::internal_error)
        {
            // Re-start
            ADD_LOG("Re-start DataFeedBinanceBookTicker");
            this->start();
        }
    });

    m_websocket->run();
}

void DataFeedBinanceBookTicker::set_call_back(std::function<void(const std::string& symbol, Json& ticker)> call_back)
{
    m_on_callback = call_back;
}

bool DataFeedBinanceBookTicker::standardize_data(const std::string& data, Json& ticker)
{
    Json book_ticker = Json::parse(data);
    if (book_ticker.has_field("s"))
    {
        // symbol
        ticker["s"] = BINANCE_SPOT_ABBREVIATION_NAME + "#" + (std::string&&)book_ticker["s"];
        // event name
        ticker["e"] = "bookTicker";
        // event time
        ticker["E"] = Utils::instance().get_time_now_in_utc_milliseconds();

        // bid price
        ticker["b"] = std::stold((std::string&&)book_ticker["b"]);
        // bid quantity
        ticker["B"] = std::stold((std::string&&)book_ticker["B"]);
        // ask price
        ticker["a"] = std::stold((std::string&&)book_ticker["a"]);
        // ask quantity
        ticker["A"] = std::stold((std::string&&)book_ticker["A"]);
        // last price
        // ticker["p"] = (stold((string&&)book_ticker["b"]) + stold((string&&)book_ticker["a"])) / 2;        

        Json a = Json::create_array();
        a.push_back(ticker["a"]);
        a.push_back(ticker["A"]);
        Json A = Json::create_array();
        A.push_back(a);
        ticker["asks"] = A;

        Json b = Json::create_array();
        b.push_back(ticker["b"]);
        b.push_back(ticker["B"]);
        Json B = Json::create_array();
        B.push_back(b);
        ticker["bids"] = B;

        return true;
    }
    return false;
}