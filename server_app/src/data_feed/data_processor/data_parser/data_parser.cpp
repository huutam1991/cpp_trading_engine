#include <data_feed/data_processor/data_parser/data_parser.h>

DataParser::DataParser(
        Json *json, 
        const string exchange_short_name, 
        const string symbol) :
    m_json(json),
    m_exchange_short_name(exchange_short_name),
    m_symbol(symbol)
{
}
