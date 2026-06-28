#include <gateways/coinbase/coinbase_gateway.h>
#include <app_utils/app_utils.h>
#include <account/account_db.h>

CoinbaseGateway::CoinbaseGateway(const std::string& key) :
    m_quoter_spot(key),
    m_quoter_perpetual(key),
    m_market_data_spot(COINBASE_ADVANCE_REALNET_WS_URL, COINBASE_ADVANCE_REALNET_WS_PORT)
    // m_market_data_perpetual(COINBASE_FUTURES_WS_URL, COINBASE_FUTURES_WS_PORT)
{
    Json account = AccountDB::load_account_by_key(key);
    m_account.from_json(account);

    bool is_testnet = account["is_testnet"];

    // Update url + port for market data SPOT
    std::string md_spot_url  = is_testnet == true ? COINBASE_ADVANCE_REALNET_WS_URL : COINBASE_ADVANCE_REALNET_WS_URL;
    std::string md_spot_port = is_testnet == true ? COINBASE_ADVANCE_REALNET_WS_PORT : COINBASE_ADVANCE_REALNET_WS_PORT;
    m_market_data_spot.update_url_and_port(md_spot_url, md_spot_port);

    // // Update url + port for market data PERPETUAL
    // std::string md_perpetual_url  = is_testnet == true ? COINBASE_TESTNET_FUTURES_WS_URL  : COINBASE_FUTURES_WS_URL;
    // std::string md_perpetual_port = is_testnet == true ? COINBASE_TESTNET_FUTURES_WS_PORT : COINBASE_FUTURES_WS_PORT;
    // m_market_data_perpetual.update_url_and_port(md_perpetual_url, md_perpetual_port);

    // m_symbols_info["spot"] = get_spot_symbols_info();
    // m_symbols_info["perpetual"] = get_perpetual_symbols_info();
}

Json CoinbaseGateway::get_spot_symbols_info()
{
    return {};
}

Json CoinbaseGateway::get_perpetual_symbols_info()
{
    return {};
}

size_t CoinbaseGateway::get_rounded_number(const std::string& lot_size)
{
    int pos_1 = lot_size.find_first_of("1"); // find the position of charater '1'
    return pos_1 - 1;
}

std::string CoinbaseGateway::round_string_number(const std::string& str_number, size_t precision)
{
    int point_pos = str_number.find_first_of(".");
    if (point_pos > -1)
    {
        return str_number.substr(0, point_pos + (precision == 0 ? 0 : precision + 1));
    }

    return str_number;
}

void CoinbaseGateway::subscribe_instruments(std::vector<const Instrument*> instruments)
{
    // Spot
    // m_market_data_spot.subscribe_symbol(symbols, [this](const std::string& symbol, Json& payload)
    // {
    //     this->on_depth_update(symbol, payload);
    // });
    // m_market_data_spot.start();

    // Perpetual
    // m_market_data_perpetual.subscribe_symbol(symbol, [this](const std::string& symbol, Json& payload)
    // {
    //     this->on_depth_update(symbol, payload);
    // });
    // m_market_data_perpetual.start();
}

void CoinbaseGateway::on_depth_update(const std::string& symbol, Json& payload)
{
    double best_bid = payload["bids"][0][0];
    double best_ask = payload["asks"][0][0];
}

ExchangeId CoinbaseGateway::get_exchange()
{
    return ExchangeId::COINBASE;
}

Task<std::unordered_set<OrderId>> CoinbaseGateway::get_open_orders_on_exchange(std::string symbol)
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

Task<void> CoinbaseGateway::cancel_all_on_exchange(std::string symbol)
{
    // Currently, only implement for SPOT
    co_await m_quoter_spot.cancel_all(std::move(symbol));

    co_return;
}

Task<Json> CoinbaseGateway::cancel_on_exchange(Order order)
{
    // Currently, only implement for SPOT
    co_return co_await m_quoter_spot.cancel(std::move(order));
}

Task<Json> CoinbaseGateway::place_on_exchange(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    CoinbaseQuoter* quoter = order.instrument->instrument_type == InstrumentType::SPOT ?
        (CoinbaseQuoter*)&m_quoter_spot :
        (CoinbaseQuoter*)&m_quoter_perpetual;

    Json response = co_await quoter->place(order);

    // Check if order is rejected
    if (response.has_field("code") && response["code"].is_object() == false && (long)response["code"] < 0)
    {
        order.status = Order::REJECTED;
        OrderManager::instance().update_order(order);
    }

    // co_return quoter->get_trade_result_from_response(response);

    co_return response;
}

Json CoinbaseGateway::get_status()
{
    Json status;
    status["status"] = "Disconnected";
    status["environment"] = "Production";
    status["endpoints"] = {
        {"spot", "https://" + std::string(COINBASE_REALNET_URL)},
    };
    status["exchange_id"] = enum_reflect::enum_name(ExchangeId::COINBASE);
    status["instruments"] = Instrument::get_instrument_list(ExchangeId::COINBASE, InstrumentType::SPOT).size();
    status["latency"] = "0ms";
    status["up_time"] = "0";
    status["accounts"] = 0;
    status["messages_per_minute"] = 0;


    return status;
}

Task<Json> CoinbaseGateway::get_balances()
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

Task<Json> CoinbaseGateway::get_positions()
{
    co_return {};
}