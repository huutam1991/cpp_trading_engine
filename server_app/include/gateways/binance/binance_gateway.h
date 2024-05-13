#ifndef BINANCE_GATEWAY_H
#define BINANCE_GATEWAY_H

#include <gateways/gateway.h>
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>

class BinanceGateway : public Gateway
{
    BinanceQuoterSpot m_quoter_spot;

public:
    BinanceGateway(const std::string& key);
    virtual Json place(Order order) override;

};

#endif //BINANCE_GATEWAY_H