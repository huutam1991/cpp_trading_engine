#include <network/https_client_request/https_client_request.h>

#include <gateways/binance/binance_order_entry.h>
#include <app_utils/app_utils.h>
#include <account/account_db.h>

BinanceOrderEntry::BinanceOrderEntry(const std::string& key, EventBase* event_base) :
    OrderEntry(ExchangeId::BINANCE, event_base),
    m_quoter_spot(key),
    m_quoter_perpetual(key)
{
    Json account = AccountDB::load_account_by_key(key);
    m_account.from_json(account);
    bool is_testnet = account["is_testnet"];
}

size_t BinanceOrderEntry::get_rounded_number(const std::string& lot_size)
{
    int pos_1 = lot_size.find_first_of("1"); // find the position of charater '1'
    return pos_1 == 0 ? 0 : pos_1 - 1;
}

std::string BinanceOrderEntry::round_string_number(const std::string& str_number, size_t precision)
{
    int point_pos = str_number.find_first_of(".");
    if (point_pos > -1)
    {
        return str_number.substr(0, point_pos + (precision == 0 ? 0 : precision + 1));
    }

    return str_number;
}

Task<std::unordered_set<OrderId>> BinanceOrderEntry::get_open_orders_on_exchange(std::string symbol)
{
    std::unordered_set<OrderId> res;

    // Currently, only implement for SPOT
    Json open_orders = co_await m_quoter_spot.get_open_orders(std::move(symbol));

    // Add order_id to [res]
    if (open_orders.is_array() == true)
    {
        open_orders.for_each([&res](Json& order)
        {
            if (order.has_field("clientOrderId"))
            {
                OrderId order_id = AppUtils::parse_order_id(order["clientOrderId"]);

                if (order_id != 0)
                {
                    res.insert(order_id);
                }
            }
        });

    }

    co_return res;
}

Task<void> BinanceOrderEntry::cancel_all_on_exchange(std::string symbol)
{
    co_await m_quoter_spot.cancel_all(symbol);
    co_await m_quoter_perpetual.cancel_all(symbol);

    co_return;
}

Task<Json> BinanceOrderEntry::cancel_on_exchange(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.instrument->instrument_type == InstrumentType::SPOT ?
        (BinanceQuoter*)&m_quoter_spot :
        (BinanceQuoter*)&m_quoter_perpetual;

    co_return co_await quoter->cancel(std::move(order));
}

Task<Json> BinanceOrderEntry::place_on_exchange(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.instrument->instrument_type == InstrumentType::SPOT ?
        (BinanceQuoter*)&m_quoter_spot :
        (BinanceQuoter*)&m_quoter_perpetual;

    Json response = co_await quoter->place(order);

    // Check if order is rejected
    if (response.has_field("code") && response["code"].is_object() == false && (long)response["code"] < 0)
    {
        order.status = Order::REJECTED;
        order.error.code = (int)response["code"];
        order.error.message = response["msg"];
        order.last_updated = Utils::get_time_now_in_utc_nanoseconds();
        OrderManager::instance().update_order(order);
    }

    co_return response;
}

Task<Json> BinanceOrderEntry::get_balances()
{
    Json balances = co_await m_quoter_spot.get_balances();

    balances["balances"].for_each([](Json& balance)
    {
        balance["available"] = std::stod((std::string)balance["free"]) + std::stod((std::string)balance["locked"]);

        balance.remove_field("btcValuation");
        balance.remove_field("withdrawing");
        balance.remove_field("ipoable");
        balance.remove_field("locked");
        balance.remove_field("freeze");
        balance.remove_field("free");
    });

    co_return balances["balances"];
}

Task<Json> BinanceOrderEntry::get_positions()
{
    Json positions = co_await m_quoter_perpetual.get_positions();
    if (positions.is_array() == false)
    {
        co_return {};
    }

    Json data;

    positions.for_each([&data](Json& position)
    {
        const Instrument* instrument = Instrument::get_instrument_by_exchange_symbol(
            ExchangeId::BINANCE,
            InstrumentType::PERPETUAL,
            (std::string)position["symbol"]
        );

        double position_amt = std::stod((std::string)position["positionAmt"]);
        std::string side = position_amt > 0 ? "LONG" : (position_amt < 0 ? "SHORT" : "FLAT");

        Json p;
        p["instrument"] = instrument->to_json();
        p["pnl"] = position["unRealizedProfit"];
        p["entry_price"] = position["entryPrice"];
        p["mark_price"] = position["markPrice"];
        p["position_amt"] = position_amt;
        p["side"] = side;

        data.push_back(p);
    });

    co_return data;
}
