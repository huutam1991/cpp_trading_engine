#include <openssl/hmac.h>
#include <string.h>
#include <external_request/https_client_async.h>
#include <ioc_pool.h>
#include <mongo_db/mongo_db.h>

#include <gateways/coinbase/coinbase_quoter/coinbase_quoter.h>
#include <account/account.h>

CoinbaseQuoter::CoinbaseQuoter(const std::string& key) : m_key{key}
{
    Json account = Account::load_account_by_key(key);
    m_api_key = std::string(account["api_key"]);
    m_api_secret = std::string(account["api_secret"]);
    m_is_testnet = (bool)account["is_testnet"];

    ADD_LOG("Coinbase account - m_api_key: " << m_api_key);
    ADD_LOG("Coinbase account - m_api_secret: " << m_api_secret);
    ADD_LOG("Coinbase account - m_is_testnet: " << m_is_testnet);
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

void CoinbaseQuoter::check_save_resonse_error(Json& response, const std::string& query, RequestMethod method)
{
    if (response.has_field("code") && response["code"].is_object() == false && (long)response["code"] < 0)
    {
        Json error;
        error["query"] = query;
        error["method"] = request_method_map_string.at((size_t)method);
        error["response"] = response;

        MongoDB::instance()
            .set_db_and_collection(STRATEGY_DB_NAME, "error")
            .insert_one(error);
    }
    else
    {
        // Only update field code = 0 for object
        if (response.is_array() == false)
        {
            response["code"] = 0;
        }
    }
}

Task<Json> CoinbaseQuoter::send_coinbase_request(RequestMethod method, std::string api_path, std::string query_str)
{
    std::string new_query_std = query_str;
    auto timestamp = getTimestamp();
    new_query_std += "&timestamp=" + timestamp;
    auto signature = getSignature(new_query_std);
    new_query_std += "&signature=" + signature;

    auto client = std::make_shared<HttpsClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), get_url(), get_port());
    client->add_header("X-MBX-APIKEY", m_api_key);
    
    std::string str_response;
    if (method == RequestMethod::GET)
    {
        str_response = co_await client->get(api_path + "?" + new_query_std);
    }
    else if (method == RequestMethod::POST)
    {
        str_response = co_await client->post(api_path + "?" + new_query_std, "");
    }
    else if (method == RequestMethod::DELETE)
    {
        str_response = co_await client->del(api_path + "?" + new_query_std, "");
    }
    else if (method == RequestMethod::PUT)
    {
        str_response = co_await client->put(api_path + "?" + new_query_std, "");
    }
    
    Json response = Json::parse(str_response);

    // Check to save error
    check_save_resonse_error(response, new_query_std, method);

    co_return response;
}
