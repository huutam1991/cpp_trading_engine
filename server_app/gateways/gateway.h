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

class Gateway
{
protected:
    ExchangeId m_exchange_id;
    EventBase* m_event_base = nullptr;

    Gateway();
    virtual std::vector<Instrument> fetch_instruments();
    virtual std::shared_ptr<OrderEntry> get_order_entry() = 0;

public:
    virtual ExchangeId get_exchange() = 0;
    std::string get_name();

    void init();

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) = 0;
    virtual void subscribe_instrument(const Instrument* instrument) = 0;
    virtual void unsubscribe_instrument(const Instrument* instrument) = 0;

    // Util methods
    virtual Json get_status() = 0;
};
