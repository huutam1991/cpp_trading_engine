#include <gateways/coinbase/coinbase_gateway.h>
#include <app_utils/app_utils.h>
#include <account/account_db.h>

CoinbaseGateway::CoinbaseGateway() :
    Gateway(ExchangeId::COINBASE),
    m_quoter_spot(""),
    m_quoter_perpetual(""),
    m_market_data_spot(COINBASE_ADVANCE_REALNET_WS_URL, COINBASE_ADVANCE_REALNET_WS_PORT)
    // m_market_data_perpetual(COINBASE_FUTURES_WS_URL, COINBASE_FUTURES_WS_PORT)
{
    m_market_data_spot.update_url_and_port(COINBASE_ADVANCE_REALNET_WS_URL, COINBASE_ADVANCE_REALNET_WS_PORT);

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

std::shared_ptr<OrderEntry> CoinbaseGateway::get_order_entry(std::shared_ptr<AccountBase> account)
{
    return nullptr;
}

std::expected<bool, std::string> CoinbaseGateway::validate_account(std::shared_ptr<AccountBase> account)
{
    // Implementation of account validation
    return true;
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
    status["accounts"] = m_accounts.size();
    status["messages_per_minute"] = 0;


    return status;
}
