#include <gateways/binance/binance_market_data/binance_market_data.h>
#include <timer.h>
#include <mongo_db/mongo_db.h>
#include <coroutine/event_base_manager.h>

#include <app_constants.h>

BinanceMarketData::BinanceMarketData(const std::string& url, const std::string& port):
    m_url(url),
    m_port(port)
{
    // Set period time to check websocket stream is stop at every 10 seconds
    add_timer_to_check_websocket_stream_is_stop(10000);
}

BinanceMarketData::~BinanceMarketData()
{
    del_timer_to_check_websocket_stream_is_stop();
    ADD_LOG("~BinanceMarketData, " << m_symbol);
}

void BinanceMarketData::start()
{
    m_websocket = std::make_shared<WebsocketClientAsync>(EventBaseManager::instance()
        .get_event_base_by_id(EventBaseID::GATEWAY) // Default is GATEWAY
    );

    m_websocket->set_callbacks(
        // on_connect
        [this]() -> TaskVoid
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

            m_websocket->send(subcribe.get_string_value());

            co_return;
        },
        // on_message
        [this](std::string buffer) -> TaskVoid
        {
            // Increase m_websocket_data_counter (should find a better solution)
            m_websocket_data_counter++;

            Json depth = Json();
            if (this->standardize_data(buffer, depth))
            {
                ADD_LOG("Stream depth: " << this->m_symbol);
                if (m_on_callback != nullptr)
                {
                    m_on_callback(m_symbol, depth);
                }
            }
            else
            {
                // Save this none json data for checking error
                MongoDB::instance()
                    .set_db_and_collection(STRATEGY_DB_NAME, "websocket_invalid_market_data")
                    .insert_one(Json::parse(buffer));
            }

            co_return;
        },
        // on_disconnect
        [this]() -> TaskVoid
        {
            // Re-start
            ADD_LOG("Disconnect, re-start BinanceMarketData");
            this->start();

            co_return;
        },
        // on_close
        []() -> TaskVoid
        {
            ADD_LOG("BinanceMarketData close");
            co_return;
        }
    );

    m_websocket->connect(m_url, m_port, "/ws");
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

void BinanceMarketData::add_timer_to_check_websocket_stream_is_stop(size_t period)
{
    m_schedule_task_id = Timer::instance().add_schedule_task([this]()
    {
        if (m_websocket_data_counter == 0)
        {
            this->start();
        }

        m_websocket_data_counter = 0;
    },
    period);
}

void BinanceMarketData::del_timer_to_check_websocket_stream_is_stop()
{
    if (m_schedule_task_id != 0)
    {
        Timer::instance().delete_schedule_task(this->m_schedule_task_id);
    }
}
