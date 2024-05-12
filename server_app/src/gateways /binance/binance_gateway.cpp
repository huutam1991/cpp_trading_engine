#include <openssl/hmac.h>
#include <external_request/external_request_ssl.h>

#include <gateways/binance/binance_gateway.h>
#include <account/account.h>

BinanceGateway::BinanceGateway(const std::string& key) : m_key{key}
{
    Json account = Account::load_account_by_key(key);
    m_api_key = std::string(account["api_key"]);
    m_api_secret = std::string(account["api_secret"]);

    ADD_LOG("Binance account - m_api_key: " << m_api_key);
    ADD_LOG("Binance account - m_api_secret: " << m_api_secret);
}

std::string BinanceGateway::getTimestamp()
{
	long long ms_since_epoch = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return std::to_string(ms_since_epoch);
}

std::string BinanceGateway::encryptWithHMAC(const char* key, const char* data)
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

std::string BinanceGateway::getSignature(std::string& query)
{
	return encryptWithHMAC(m_api_secret.c_str(), query.c_str());
}

Json BinanceGateway::send_binance_request(RequestMethod method, const std::string& api_path, const std::string& query_str)
{
    std::string new_query_std = query_str;
    auto timestamp = getTimestamp();
    new_query_std += "&timestamp=" + timestamp;
    auto signature = getSignature(new_query_std);
    new_query_std += "&signature=" + getSignature(new_query_std);

    ExternalRequestSsl binance_request(m_url, m_port, api_path + "?" + new_query_std, method);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    return Json::parse(binance_request.send_request(""));
}

void BinanceGateway::place(Order order)
{
    // /api/v3/order?symbol=BTCUSDT&type=LIMIT&timeInForce=GTC&quantity=0.001&recvWindow=15000&price=19840&side=BUY
    std::string query_str;
    std::string side = order.side == Order::Side::BUY ? "BUY" : "SELL";

    query_str += "symbol=" + order.symbol;
    query_str += "&side=" + side;
    query_str += "&type=" + order.type;
    query_str += "&quantity=" + std::to_string(order.quantity);
    if (order.type == "LIMIT")
    {
        query_str += "&timeInForce=GTC";
        query_str += "&price=" + std::to_string(order.price);
    }

    Json response = send_binance_request(RequestMethod::POST, "/api/v3/order", query_str);

    ADD_LOG("Binance place order: " << response);
}