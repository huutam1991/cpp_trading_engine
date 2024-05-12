#ifndef BINANCE_GATEWAY_H
#define BINANCE_GATEWAY_H

#include <gateways/gateway.h>

class BinanceGateway : public Gateway
{
private:
    std::string m_key;
    std::string m_api_key;
    std::string m_api_secret;

public:
    BinanceGateway(const std::string& key);
    virtual void place(Order order) override;

};

#endif //BINANCE_GATEWAY_H