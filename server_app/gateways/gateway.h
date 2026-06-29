#pragma once

#include <unordered_set>
#include <spdlog/spdlog.h>

#include <utils/util_macros.h>
#include <json/json.h>
#include <coroutine/task.h>
#include <data_model/savable_object.h>
#include <network/external_request/https_client_async.h>

#include <instrument/instrument.h>
#include <order/order_manager.h>
#include <app_constants.h>
#include <gateways/order_entry.h>
#include <account/account.h>

class Gateway
{
protected:
    ExchangeId m_exchange_id;
    EventBase* m_event_base = nullptr;
    std::unordered_map<std::string, std::shared_ptr<AccountBase>> m_accounts;

    Gateway();
    virtual std::vector<Instrument> fetch_instruments();
    virtual std::shared_ptr<OrderEntry> get_order_entry() = 0;

public:
    virtual ExchangeId get_exchange() = 0;
    std::string get_name();

    void init();
    void add_account(std::shared_ptr<AccountBase> account);
    void remove_account(std::shared_ptr<AccountBase> account);

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) = 0;
    virtual void subscribe_instrument(const Instrument* instrument) = 0;
    virtual void unsubscribe_instrument(const Instrument* instrument) = 0;

    // Util methods
    virtual std::expected<bool, std::string> validate_account(std::shared_ptr<AccountBase> account) = 0;
    virtual Json get_status() = 0;
};
