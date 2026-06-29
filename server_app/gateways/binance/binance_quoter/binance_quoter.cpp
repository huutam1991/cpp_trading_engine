#include <openssl/hmac.h>
#include <string.h>
#include <network/https_client_request/https_client_request.h>
#include <mongo_db/mongo_db.h>

#include <gateways/binance/binance_quoter/binance_quoter.h>
#include <account/account_db.h>

BinanceQuoter::BinanceQuoter(const std::string& key) : m_key{key}
{
    Json account = AccountDB::load_account_by_key(key);
    m_api_key = std::string(account["api_key"]);
    m_api_secret = std::string(account["api_secret"]);
    m_is_testnet = (bool)account["is_testnet"];

    spdlog::debug("Binance account - m_api_key: {}", m_api_key);
    spdlog::debug("Binance account - m_api_secret: {}", m_api_secret);
    spdlog::debug("Binance account - m_is_testnet: {}", m_is_testnet);
}

Task<Json> BinanceQuoter::get_balances()
{
    HttpsClientRequest client(m_epoll_base, get_url(), std::stoi(get_port()));
    Json ba = co_await send_binance_request(RequestMethod::GET, "/api/v3/account", "", &client);
    spdlog::debug("BinanceQuoter::get_balances - response: {}", ba);

    co_return ba;
}

Task<Json> BinanceQuoter::get_positions()
{
    HttpsClientRequest client(m_epoll_base, get_url(), std::stoi(get_port()));
    client.add_header("X-MBX-APIKEY", m_api_key);
    co_return co_await send_binance_request(RequestMethod::GET, "/fapi/v3/positionRisk", "", &client);
}

std::string BinanceQuoter::getTimestamp()
{
	long long ms_since_epoch = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return std::to_string(ms_since_epoch);
}

std::string BinanceQuoter::encryptWithHMAC(const char* key, const char* data)
{
    static constexpr char hex[] = "0123456789abcdef";

    unsigned int result_len = 0;
    unsigned char result[EVP_MAX_MD_SIZE];

    unsigned char* hmac_res = HMAC(
        EVP_sha256(),
        key,
        static_cast<int>(strlen(key)),
        reinterpret_cast<const unsigned char*>(data),
        strlen(data),
        result,
        &result_len
    );

    if (hmac_res == nullptr || result_len != 32)
    {
        throw std::runtime_error("HMAC-SHA256 failed");
    }

    std::string signature;
    signature.resize(result_len * 2);

    for (unsigned int i = 0; i < result_len; ++i)
    {
        signature[2 * i]     = hex[(result[i] >> 4) & 0x0F];
        signature[2 * i + 1] = hex[result[i] & 0x0F];
    }

    return signature;
}

std::string BinanceQuoter::getSignature(std::string& query)
{
	return encryptWithHMAC(m_api_secret.c_str(), query.c_str());
}

Task<Json> BinanceQuoter::send_binance_request(RequestMethod method, std::string api_path, std::string query_str, HttpsClientRequest* client)
{
    std::string new_query_std = query_str;
    auto timestamp = getTimestamp();
    new_query_std += "&timestamp=" + timestamp;
    auto signature = getSignature(new_query_std);

    new_query_std += "&signature=" + signature;

    HttpsClientResponse response;
    if (method == RequestMethod::GET)
    {
        response = co_await client->get(api_path + "?" + new_query_std);
    }
    else if (method == RequestMethod::POST)
    {
        response = co_await client->post(api_path + "?" + new_query_std, "");
    }
    else if (method == RequestMethod::DELETE)
    {
        response = co_await client->del(api_path + "?" + new_query_std);
    }
    else if (method == RequestMethod::PUT)
    {
        response = co_await client->put(api_path + "?" + new_query_std, "");
    }

    Json response_json = Json::parse(response.body);
    if (response.status_code == -1)
    {
        response_json = {
            {"code", -1},
            {"msg", "Disconnected"}
        };
    }

    co_return response_json;
}
