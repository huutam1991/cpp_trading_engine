#include <gateways/binance/binance_gateway.h>
#include <account/account.h>

BinanceGateway::BinanceGateway(const std::string& key) : m_key{key}
{
    Json account = Account::load_account_by_key(key);
    m_api_key = std::string(account["api_key"]);
    m_api_secret = std::string(account["api_secret"]);

    ADD_LOG("Binance account - m_api_key: " << m_api_key);
    ADD_LOG("Binance account - m_api_secret: " << m_api_secret);
}

void BinanceGateway::place(Order order)
{
    // TBD
}