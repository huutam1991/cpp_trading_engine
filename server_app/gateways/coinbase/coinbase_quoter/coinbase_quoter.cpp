#include <openssl/hmac.h>
#include <string.h>
#include <network/external_request/https_client_async.h>
#include <mongo_db/mongo_db.h>

#include <gateways/coinbase/coinbase_quoter/coinbase_quoter.h>
#include <account/account.h>

CoinbaseQuoter::CoinbaseQuoter(const std::string& key) : m_key{key}
{
    Json account = Account::load_account_by_key(key);
    m_api_key = std::string(account["api_key"]);
    m_api_secret = std::string(account["api_secret"]);
    m_is_testnet = (bool)account["is_testnet"];
}

Task<Json> CoinbaseQuoter::get_balances()
{
    co_return co_await send_coinbase_request(RequestMethod::GET, "/api/v3/account", "");
}

std::string CoinbaseQuoter::getTimestamp()
{
	long long ms_since_epoch = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return std::to_string(ms_since_epoch);
}

std::string CoinbaseQuoter::encryptWithHMAC(const char* key, const char* data)
{
    unsigned char *result;
    static char res_hexstring[64];
    int result_len = 32;
    std::string signature;

    result = HMAC(EVP_sha256(), key, strlen((char *)key), const_cast<unsigned char *>(reinterpret_cast<const unsigned char*>(data)), strlen((char *)data), NULL, NULL);
  	for (int i = 0; i < result_len; i++)
    {
    	sprintf(&(res_hexstring[i * 2]), "%02x", result[i]);
  	}

  	for (int i = 0; i < 64; i++)
    {
  		signature += res_hexstring[i];
  	}

  	return signature;
}

std::string CoinbaseQuoter::getSignature(std::string& query)
{
	return encryptWithHMAC(m_api_secret.c_str(), query.c_str());
}

Task<Json> CoinbaseQuoter::send_coinbase_request(RequestMethod method, std::string api_path, std::string query_str)
{
    co_return {};
}
