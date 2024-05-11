#ifndef BINANCE_FUTURE_PARSER_H
#define BINANCE_FUTURE_PARSER_H

#include "data_parser.h"

class BinanceFutureParser : DataParser
{
public:
    BinanceFutureParser(
        Json *json, 
        const string exchange_short_name, 
        const string symbol) : 
        DataParser(json, exchange_short_name, symbol) {};

    void parse_order_book(const std::string& buffer) override;

private:

};

#endif