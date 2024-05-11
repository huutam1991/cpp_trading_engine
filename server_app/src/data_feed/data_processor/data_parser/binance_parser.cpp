#include <data_feed/data_processor/data_parser/binance_parser.h>
#include <utils.h>

using namespace std;

void BinanceParser::parse_order_book(const std::string& buffer)
{
    Json order_book = Json::parse(buffer);
    // parse book ticker
    if (order_book.has_field("s"))
    {
        // symbol
        (*m_json)["s"] = m_exchange_short_name + "#" + (string&&)order_book["s"];
        // event name
        (*m_json)["e"] = "bookTicker";
        // event time
        (*m_json)["E"] = Utils::instance().get_time_now_in_utc_milliseconds();
        // ticker info
        // bid price
        (*m_json)["b"] = stold((string&&)order_book["b"]);
        // bid quantity
        (*m_json)["B"] = stold((string&&)order_book["B"]);
        // ask price
        (*m_json)["a"] = stold((string&&)order_book["a"]);
        // ask quantity
        (*m_json)["A"] = stold((string&&)order_book["A"]);
        // depth
        Json a = Json::create_array();
        a.push_back((*m_json)["a"]);
        a.push_back((*m_json)["A"]);
        Json A = Json::create_array();
        A.push_back(a);
        (*m_json)["asks"] = A;

        Json b = Json::create_array();
        b.push_back((*m_json)["b"]);
        b.push_back((*m_json)["B"]);
        Json B = Json::create_array();
        B.push_back(b);
        (*m_json)["bids"] = B;
    }
    
    // parse depth
    if (order_book.has_field("asks") && order_book.has_field("bids"))
    {
        // symbol
        (*m_json)["s"] = m_exchange_short_name + "#" + m_symbol;
        // event name
        (*m_json)["e"] = "depthUpdate";

        //  update asks
        Json A = Json::create_array();
        Json asks = order_book["asks"];
        asks.for_each([&A](Json& data)
        {
            Json j = Json::create_array();
            j.push_back(stold((string&&)data[0]));
            j.push_back(stold((string&&)data[1]));
            A.push_back(j);
        }
        );
        (*m_json)["asks"] = A;

        // update bids
        Json B = Json::create_array();
        Json bids = order_book["bids"];
        bids.for_each([&B](Json& data)
        {
            Json j = Json::create_array();
            j.push_back(stold((string&&)data[0]));
            j.push_back(stold((string&&)data[1]));
            B.push_back(j);
        }
        );
        (*m_json)["bids"] = B;
    }
}