#include <gateways/binance/binance_market_data/binance_market_data.h>
#include <timer.h>

BinanceMarketData::BinanceMarketData(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    add_timer_reset_websocket(1800000);
}

BinanceMarketData::~BinanceMarketData()
{
    ADD_LOG("~BinanceMarketData, " << m_symbol);
    del_timer_reset_websocket();
}

void BinanceMarketData::start()
{
    m_websocket = std::make_shared<WebsocketClient>(m_url, m_port, "/ws");

    m_websocket->on_connect([this](WebsocketClientHandle& ws)
    {
        ADD_LOG("Binance websocket depth connected");

        // Subcribe for depth
        size_t stream_id = get_stream_id_count();
        std::string lower_case_symbol = m_symbol;
        STRING_LOWER_CASE(lower_case_symbol);

        Json params;
        params[0] = lower_case_symbol + "@depth5@1000ms";

        Json subcribe;
        subcribe["method"] = "SUBSCRIBE";
        subcribe["params"] = params;
        subcribe["id"] = stream_id;

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
            ADD_LOG("Re-start BinanceMarketData");
            this->start();
        }
    });

    m_websocket->run();
}

size_t BinanceMarketData::get_stream_id_count()
{
    static int stream_id_count = 0;
    return ++stream_id_count;
}

void BinanceMarketData::update_url_and_port(const std::string& url, const std::string& port)
{
    m_url = url;
    m_port = port;
}

void BinanceMarketData::subscribe_symbol(const std::string& symbol, std::function<void(const std::string& symbol, Json& payload)> call_back)
{
    m_symbol = symbol;
    m_on_callback = call_back;
}

bool BinanceMarketData::standardize_data(const std::string& data, Json& depth)
{
    Json order_book = Json::parse(data);
    if (order_book.has_field("asks") && order_book.has_field("bids"))
    {
        // symbol
        depth["s"] = m_symbol;
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

void BinanceMarketData::add_timer_reset_websocket(size_t period)
{
    m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    {
        start();
    },
    period);
}

void BinanceMarketData::del_timer_reset_websocket()
{
    if (m_schedule_task_id != 0)
    {
        Timer::instance().delete_schedule_task(this->m_schedule_task_id);
    }
}