#ifndef BINANCE_PARSER_H
#define BINANCE_PARSER_H

#include "data_parser.h"

class BinanceParser : DataParser
{
public:
    BinanceParser(
        Json *json, 
        const string exchange_short_name, 
        const string symbol) : 
        DataParser(json, exchange_short_name, symbol) {};

    void parse_order_book(const std::string& buffer) override;

private:

};

#endif