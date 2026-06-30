#pragma once

#include <coroutine/event_base_manager.h>

#include <gateways/binance/binance_gateway.h>

class BinanceTestnetGateway : public BinanceGateway
{
public:
    BinanceTestnetGateway();

protected:
    virtual std::shared_ptr<OrderEntry> get_order_entry(std::shared_ptr<AccountBase> account) override;
};
