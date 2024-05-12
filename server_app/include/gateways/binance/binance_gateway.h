#ifndef BINANCE_GATEWAY_H
#define BINANCE_GATEWAY_H

#include <gateways/gateway.h>

class BinanceGateway : public Gateway
{
public:
    virtual void place(Order order) override;

};

#endif //BINANCE_GATEWAY_H