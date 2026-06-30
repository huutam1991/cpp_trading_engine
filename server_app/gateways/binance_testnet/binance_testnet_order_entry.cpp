#include <network/https_client_request/https_client_request.h>

#include <gateways/binance_testnet/binance_testnet_order_entry.h>
#include <app_utils/app_utils.h>
#include <account/account_db.h>

BinanceTestnetOrderEntry::BinanceTestnetOrderEntry(std::shared_ptr<AccountBase> account, EventBase* event_base) :
    BinanceOrderEntry(ExchangeId::BINANCE_TESTNET, account, event_base)
{
}
