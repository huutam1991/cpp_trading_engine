
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>

BinanceQuoterSpot::BinanceQuoterSpot(const std::string& key) : BinanceQuoter(key)
{
}

std::string& BinanceQuoterSpot::get_url()
{
    return m_url;
}

std::string& BinanceQuoterSpot::get_port()
{
    return m_port;
}