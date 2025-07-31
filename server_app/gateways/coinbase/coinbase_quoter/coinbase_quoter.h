#pragma once

#include <gateways/gateway.h>

class CoinbaseQuoter
{
    std::string getTimestamp();
    std::string getSignature(std::string& query);
    std::string encryptWithHMAC(const char* key, const char* data);

protected:
    std::string m_key;
    std::string m_api_key;
    std::string m_api_secret;

    bool m_is_testnet = false;
    virtual std::string& get_url() = 0;
    virtual std::string& get_port() = 0;

    Task<JsonNew> send_coinbase_request(RequestMethod method, std::string api_path, std::string query_str);
    void check_save_resonse_error(JsonNew& response, const std::string& query, RequestMethod method);

public:
    CoinbaseQuoter(const std::string& key);

    Task<JsonNew> get_balances();

    virtual JsonNew get_trade_result_from_response(JsonNew& response) = 0;
    virtual Task<JsonNew> get_open_orders(std::string symbol) = 0;
    virtual TaskVoid cancel_all(std::string symbol) = 0;
    virtual Task<JsonNew> cancel(Order order) = 0;
    virtual Task<JsonNew> place(Order order) = 0;

};
