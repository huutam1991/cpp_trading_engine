#ifndef BINANCE_QUOTER_H
#define BINANCE_QUOTER_H

#include <gateways/gateway.h>

class BinanceQuoter
{
    std::string m_key;
    std::string m_api_key;
    std::string m_api_secret;

    std::string getTimestamp();
    std::string getSignature(std::string& query);
    std::string encryptWithHMAC(const char* key, const char* data);

protected:
    virtual std::string& get_url() = 0;
    virtual std::string& get_port() = 0;

    Json send_binance_request(RequestMethod method, const std::string& api_path, const std::string& query_str);

public:
    BinanceQuoter(const std::string& key);

    virtual Json get_trade_result_from_response(Json& response) = 0;
    virtual Json place(Order order) = 0;

};

#endif //BINANCE_QUOTER_H