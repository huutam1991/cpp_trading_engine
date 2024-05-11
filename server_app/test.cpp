#include <ring_buffer/ring_buffer.h>
#include <json/json.h>
#include <iostream>
#include <exchanges/exchange_gateway.h>

using namespace std;

void testExchangeGateway()
{
    ExchangeGateWay::instance().subscribe_data(
                BINANCE_SPOT,
                MARKET_INFO,
                "BTCDOWN",
                [](const std::string& symbol, Json& payload)
        {
            cout << payload.get_string_value() << endl;
        });
}

void testMarketScanningStrategy()
{
}

void testRingBufferWithInt()
{
    RingBuffer<int> buffer = RingBuffer<int>(5); // buffer capacity is 5

    auto print_buffer = [&]() -> void {
        for (int i = 0; i < buffer.size(); i++)
        {
            cout << buffer[i] << " ";
        }
        cout << endl;
        
        return;
    };

    // all of the following return true
    buffer.unshift(1); // [1] 
    buffer.unshift(2); // [2,1]
    buffer.unshift(3); // [3,2,1]
    print_buffer();
    buffer.push(0);  // [3,2,1,0]
    buffer.push(5);  // [3,2,1,0,5]
    print_buffer();

    buffer.unshift(2);  // [2,3,2,1,0] returns false
    print_buffer();
    buffer.unshift(10); // [10,2,3,2,1] returns false
    print_buffer();
    buffer.push(-5);  // [2,3,2,1,-5] returns false
    print_buffer();
} 


void testRingBufferWithJson()
{
    RingBuffer<Json> buffer = RingBuffer<Json>(5);
    auto print_buffer = [&]() -> void {
        for (int i = 0; i < buffer.size(); i++)
        {
            Json j = Json(buffer[i]);
            cout << j.get_string_value() << endl;
        }        
        return;
    };
    {
        Json j1;
        j1["b"] = 1.0;
        j1["B"] = 1.0;
        j1["a"] = 1.0;
        j1["A"] = 1.0;
        j1["T"] = 111111111;

        Json a1 = Json::create_array();
        a1.push_back(1);
        a1.push_back(1);
        Json a2 = Json::create_array();
        a2.push_back(2);
        a2.push_back(2);
        Json a3 = Json::create_array();
        a3.push_back(3);
        a3.push_back(3);
        Json A1 = Json::create_array();
        A1.push_back(a1);
        A1.push_back(a2);
        A1.push_back(a3);
        j1["asks"] = A1;

        Json b1 = Json::create_array();
        b1.push_back(1);
        b1.push_back(1);
        Json b2 = Json::create_array();
        b2.push_back(2);
        b2.push_back(2);
        Json b3 = Json::create_array();
        b3.push_back(3);
        b3.push_back(3);
        Json B1 = Json::create_array();
        B1.push_back(b1);
        B1.push_back(b2);
        B1.push_back(b3);
        j1["bids"] = B1;

        buffer.unshift(j1);

        Json j2;
        j2["b"] = 2.0;
        j2["B"] = 2.0;
        j2["a"] = 2.0;
        j2["A"] = 2.0;
        j2["T"] = 22222222;
        a1 = Json::create_array();
        a1.push_back(11);
        a1.push_back(11);
        a2 = Json::create_array();
        a2.push_back(22);
        a2.push_back(22);
        a3 = Json::create_array();
        a3.push_back(33);
        a3.push_back(33);
        A1 = Json::create_array();
        A1.push_back(a1);
        A1.push_back(a2);
        A1.push_back(a3);
        j2["asks"] = A1;

        b1 = Json::create_array();
        b1.push_back(11);
        b1.push_back(11);
        b2 = Json::create_array();
        b2.push_back(22);
        b2.push_back(22);
        b3 = Json::create_array();
        b3.push_back(33);
        b3.push_back(33);
        B1 = Json::create_array();
        B1.push_back(b1);
        B1.push_back(b2);
        B1.push_back(b3);
        j2["bids"] = B1;

        buffer.unshift(j2);
    }
    print_buffer();
}