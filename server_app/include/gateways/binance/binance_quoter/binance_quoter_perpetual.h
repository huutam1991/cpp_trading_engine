#ifndef BINANCE_QUOTER_PERPETUAL_H
#define BINANCE_QUOTER_PERPETUAL_H

#include <gateways/binance/binance_quoter/binance_quoter.h>

class BinanceQuoterPerpetual : public BinanceQuoter
{
private:
    std::string m_url = BINANCE_FUTURES_URL;
    std::string m_port = BINANCE_FUTURES_PORT;

protected:
    virtual std::string& get_url() override;
    virtual std::string& get_port() override;

public:
    BinanceQuoterPerpetual(const std::string& key);
    virtual Json place(Order order);

};

#endif //BINANCE_QUOTER_PERPETUAL_H