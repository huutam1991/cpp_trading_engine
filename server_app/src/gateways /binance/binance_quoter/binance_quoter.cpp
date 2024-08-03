#include <openssl/hmac.h>
#include <external_request/external_request_ssl.h>
#include <mongo_db/mongo_db.h>

#include <gateways/binance/binance_quoter/binance_quoter.h>
#include <account/account.h>

BinanceQuoter::BinanceQuoter(const std::string& key) : m_key{key}
{
    Json account = Account::load_account_by_key(key);
    m_api_key = std::string(account["api_key"]);
    m_api_secret = std::string(account["api_secret"]);
    m_is_testnet = (bool)account["is_testnet"];

    ADD_LOG("Binance account - m_api_key: " << m_api_key);
    ADD_LOG("Binance account - m_api_secret: " << m_api_secret);
    ADD_LOG("Binance account - m_is_testnet: " << m_is_testnet);
}

Json BinanceQuoter::get_balances()
{
    return send_binance_request(RequestMethod::POST, "/sapi/v3/asset/getUserAsset", "");
}

std::string BinanceQuoter::getTimestamp()
{
	long long ms_since_epoch = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return std::to_string(ms_since_epoch);
}

std::string BinanceQuoter::encryptWithHMAC(const char* key, const char* data)
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

std::string BinanceQuoter::getSignature(std::string& query)
{
	return encryptWithHMAC(m_api_secret.c_str(), query.c_str());
}

void BinanceQuoter::check_save_resonse_error(Json& response, const std::string& query)
{
    if (response.has_field("code") && response["code"].is_object() == false && (long)response["code"] < 0)
    {
        Json error;
        error["query"] = query;
        error["order"] = m_order.to_json();
        error["response"] = response;

        MongoDB::instance()
            .set_db_and_collection(STRATEGY_DB_NAME, "error")
            .insert_one(error);
    }
    else
    {
        response["code"] = 0;
    }
}

Json BinanceQuoter::send_binance_request(RequestMethod method, const std::string& api_path, const std::string& query_str)
{
    std::string new_query_std = query_str;
    auto timestamp = getTimestamp();
    new_query_std += "&timestamp=" + timestamp;
    auto signature = getSignature(new_query_std);
    new_query_std += "&signature=" + getSignature(new_query_std);

    ExternalRequestSsl binance_request(get_url(), get_port(), api_path + "?" + new_query_std, method);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    Json response = Json::parse(binance_request.send_request(""));

    // Check to save error
    check_save_resonse_error(response, new_query_std);

    return response;
}
