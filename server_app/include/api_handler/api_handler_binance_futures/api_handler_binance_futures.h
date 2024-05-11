
#ifndef API_HANDLER_BINANCE_FUTURES_H
#define API_HANDLER_BINANCE_FUTURES_H

#include <api_handler/api_handler.h>

class APIHandlerBinanceFutures : public APIHandler
{
public:
    APIHandlerBinanceFutures(HttpRequest* request);

    static long long delta;
    static void synchronize_server_time();

    APIHandlerBinanceFutures& set_url(const std::string& url);
    APIHandlerBinanceFutures& set_authen_info(StorageSource* storage_source);
    Json send_binance_request(const std::string& api_path, const std::string& query_str, RequestMethod method = RequestMethod::GET);
    Json send_binance_normal_request(const std::string& api_path, const std::string& query_str, RequestMethod method = RequestMethod::GET);

protected:
    std::string m_api_key;
    std::string m_api_secret;
    std::string m_url = BINANCE_FUTURES_URL;
    std::string m_port = BINANCE_FUTURES_PORT;

    virtual HttpResponse child_handle();
    virtual std::string check_authentication();

    std::string getTimestamp();
    std::string getSignature(std::string& query);
    std::string encryptWithHMAC(const char* key, const char* data);

private:
    Json response_handler(const std::string& res);
};

#endif //API_HANDLER_BINANCE_FUTURES_H