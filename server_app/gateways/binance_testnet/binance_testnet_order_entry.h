
#include <gateways/binance/binance_order_entry.h>

class BinanceTestnetOrderEntry : public BinanceOrderEntry
{
public:
    BinanceTestnetOrderEntry(std::shared_ptr<AccountBase> account, EventBase* event_base);
};
