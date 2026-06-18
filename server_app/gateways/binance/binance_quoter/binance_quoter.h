#pragma once

#include <coroutine/event_base_manager.h>
#include <gateways/gateway.h>

class BinanceQuoter
{
    EpollBase* m_epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::EPOLL_GATEWAY);

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

    Task<Json> send_binance_request(RequestMethod method, std::string api_path, std::string query_str, HttpsClientRequest* client);

public:
    BinanceQuoter(const std::string& key);

    Task<Json> get_balances();

    virtual Task<Json> get_open_orders(std::string symbol) = 0;
    virtual Task<void> cancel_all(std::string symbol) = 0;
    virtual Task<Json> cancel(Order order) = 0;
    virtual Task<Json> place(Order order) = 0;

};
