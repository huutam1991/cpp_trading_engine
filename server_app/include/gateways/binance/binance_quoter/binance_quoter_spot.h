#ifndef BINANCE_QUOTER_SPOT_H
#define BINANCE_QUOTER_SPOT_H

#include <gateways/binance/binance_quoter/binance_quoter.h>

class BinanceQuoterSpot : public BinanceQuoter
{
private:
    std::string m_url = BINANCE_SPOT_URL;
    std::string m_port = BINANCE_SPOT_PORT;

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    BinanceQuoterSpot(const std::string& key);
    virtual Json place(Order order);

};

#endif //BINANCE_QUOTER_SPOT_H