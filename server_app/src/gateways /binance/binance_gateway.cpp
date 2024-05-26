#include <gateways/binance/binance_gateway.h>
#include <account/account.h>

BinanceGateway::BinanceGateway(const std::string& key) :
    m_quoter_spot(key),
    m_quoter_perpetual(key),
    m_market_data_spot(BINANCE_SPOT_WS_URL, BINANCE_SPOT_WS_PORT),
    m_market_data_perpetual(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT)
{
    Json account = Account::load_account_by_key(key);
    bool is_testnet = account["is_testnet"];

    // Update url + port for market data SPOT
    std::string md_spot_url  = is_testnet == true ? BINANCE_TESTNET_SPOT_WS_URL : BINANCE_SPOT_WS_URL;
    std::string md_spot_port = is_testnet == true ? BINANCE_TESTNET_SPOT_WS_PORT : BINANCE_SPOT_WS_PORT;
    m_market_data_spot.update_url_and_port(md_spot_url, md_spot_port);

    // Update url + port for market data PERPETUAL
    std::string md_perpetual_url  = is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_URL  : BINANCE_FUTURES_WS_URL;
    std::string md_perpetual_port = is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_PORT : BINANCE_FUTURES_WS_PORT;
    m_market_data_perpetual.update_url_and_port(md_perpetual_url, md_perpetual_port);
}

void BinanceGateway::subscribe_symbol(const std::string& symbol)
{
    // Spot
    m_market_data_spot.subscribe_symbol(symbol, [this](const std::string& symbol, Json& payload)
    {
        this->on_depth_update(symbol, payload);
    });
    m_market_data_spot.start();

    // Perpetual
    // m_market_data_perpetual.subscribe_symbol(symbol, [this](const std::string& symbol, Json& payload)
    // {
    //     this->on_depth_update(symbol, payload);
    // });
    // m_market_data_perpetual.start();
}

void BinanceGateway::on_depth_update(const std::string& symbol, Json& payload)
{
    double best_bid = payload["bids"][0][0];
    double best_ask = payload["asks"][0][0];
    ADD_LOG("On depth update - symbol: " << symbol << " - best_bid: " << best_bid << " - best_ask: " << best_ask);

    m_price_update_callback(best_ask);
}

Json BinanceGateway::place(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.exchange_type == Order::ExchangeType::SPOT ?
        (BinanceQuoter*)&m_quoter_spot :
        (BinanceQuoter*)&m_quoter_perpetual;

    Json response = quoter->place(order);
    // Tricky here, assume value of price from [response] is current market price
    response["price"] = order.price;

    return quoter->get_trade_result_from_response(response);
}

Json BinanceGateway::get_balances()
{
    return m_quoter_spot.get_balances();
}