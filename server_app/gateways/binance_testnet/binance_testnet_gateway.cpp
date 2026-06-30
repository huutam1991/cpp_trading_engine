#include <network/https_client_request/https_client_request.h>

#include <gateways/binance_testnet/binance_testnet_gateway.h>
#include <gateways/binance_testnet/binance_testnet_order_entry.h>

BinanceTestnetGateway::BinanceTestnetGateway() :
    BinanceGateway(ExchangeId::BINANCE_TESTNET)
{
}

std::shared_ptr<OrderEntry> BinanceTestnetGateway::get_order_entry(std::shared_ptr<AccountBase> account)
{
    return std::make_shared<BinanceTestnetOrderEntry>(account, m_epoll_base);
}