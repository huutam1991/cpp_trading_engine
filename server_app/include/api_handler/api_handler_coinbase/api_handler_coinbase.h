#ifndef API_HANDLER_COINBASE_H
#define API_HANDLER_COINBASE_H

#include <api_handler/api_handler.h>

class APIHandlerCoinbase : public APIHandler
{
protected:
    std::string m_api_key;
    std::string m_api_secret;
    std::string m_passphrase;
    // std::string m_url = COINBASE_REALNET_URL; // default Coinbase's url is real net
    std::string m_url = COINBASE_TESTNET_URL;

    virtual std::string check_authentication();
    virtual HttpResponse child_handle();

    std::string getTimestamp();
    std::string encryptWithHMAC(const std::string& key, const char* data);
    std::string getSignature(const std::string& query);

public:
    APIHandlerCoinbase(HttpRequest* request);

    static long long delta;
    static void synchronize_server_time();

    Json send_coinbase_request(const std::string& api_path, const std::string& query_str, const std::string& body, RequestMethod method = RequestMethod::GET);
    Json send_coinbase_normal_request(const std::string& api_path, const std::string& query_str, RequestMethod method = RequestMethod::GET);
    APIHandlerCoinbase& set_authen_info(StorageSource* storage_source);

};

#endif //API_HANDLER_COINBASE_H