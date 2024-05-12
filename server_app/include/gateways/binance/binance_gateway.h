#ifndef BINANCE_GATEWAY_H
#define BINANCE_GATEWAY_H

#include <gateways/gateway.h>

class BinanceGateway : public Gateway
{
private:
    std::string m_key;
    std::string m_api_key;
    std::string m_api_secret;

    std::string m_url = BINANCE_SPOT_URL;
    std::string m_port = BINANCE_SPOT_PORT;

    std::string getTimestamp();
    std::string getSignature(std::string& query);
    std::string encryptWithHMAC(const char* key, const char* data);

    Json send_binance_request(RequestMethod method, const std::string& api_path, const std::string& query_str);

public:
    BinanceGateway(const std::string& key);
    virtual void place(Order order) override;

};

#endif //BINANCE_GATEWAY_H