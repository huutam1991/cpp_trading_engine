#ifndef DATA_PARSER_H
#define DATA_PARSER_H

#include <json/json.h>
#include <string>

using namespace std;

class DataParser
{
public:
    DataParser(
        Json *json, 
        const string exchange_short_name, 
        const string symbol);
    virtual void parse_order_book(const std::string& buffer) = 0;

protected:
    Json *m_json;
    string m_exchange_short_name;
    string m_symbol;
};

#endif