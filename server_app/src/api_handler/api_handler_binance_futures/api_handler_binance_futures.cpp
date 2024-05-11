#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>
#include <storage_source/binance_futures/storage_source_binance_futures.h>
#include <exchanges/binance_futures.h>
//#include <jwt/jwt_manager.h>
#include <binance_utils.h>

APIHandlerBinanceFutures::APIHandlerBinanceFutures(HttpRequest* request) : APIHandler(request)
{
}

void APIHandlerBinanceFutures::synchronize_server_time()
{
    Json data = APIHandlerBinanceFutures(nullptr).send_binance_normal_request("/fapi/v1/time", "");

	long long ms_since_epoch = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    long long server_time = data["serverTime"];
    APIHandlerBinanceFutures::delta = server_time - ms_since_epoch;
}

std::string APIHandlerBinanceFutures::check_authentication()
{
    std::string check_valid_token = APIHandler::check_authentication();

    // Get m_url + m_api_key + m_api_secret
    if (check_valid_token == VALID_TOKEN)
    {
        set_authen_info(m_user->get_active_storage_source().get());
    }

    return check_valid_token;
}

APIHandlerBinanceFutures& APIHandlerBinanceFutures::set_authen_info(StorageSource* storage_source)
{
    if (storage_source->get_source_type() == SourceType::BINANCE_FUTURES ||
        storage_source->get_source_type() == SourceType::BINANCE_TESTNET ||
        storage_source->get_source_type() == SourceType::BINANCE_SIMULATOR)
    {
        m_api_key = ((StorageSourceBinanceFutures*)storage_source)->get_api_key();
        m_api_secret = ((StorageSourceBinanceFutures*)storage_source)->get_api_secret();
    }

    ADD_LOG(">>> authen info - source = " << storage_source->get_db_name());
    ADD_LOG(">>> authen info - url = " << m_url);
    ADD_LOG(">>> authen info - user_id = " << storage_source->get_user_id());

    return *this;
}

APIHandlerBinanceFutures& APIHandlerBinanceFutures::set_url(const std::string& url)
{
    m_url = url;
    return *this;
}

HttpResponse APIHandlerBinanceFutures::child_handle()
{
    return HttpResponse(ResponseStatusCode::OK_200, "APIHandlerBinanceFutures");
}

std::string APIHandlerBinanceFutures::getTimestamp()
{
	long long ms_since_epoch = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    ms_since_epoch += APIHandlerBinanceFutures::delta;
	return std::to_string(ms_since_epoch);
}

std::string APIHandlerBinanceFutures::encryptWithHMAC(const char* key, const char* data)
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

std::string APIHandlerBinanceFutures::getSignature(std::string& query)
{
	return encryptWithHMAC(m_api_secret.c_str(), query.c_str());
}

Json APIHandlerBinanceFutures::send_binance_request(const std::string& api_path, const std::string& query_str, RequestMethod method)
{
    std::string new_query_std = query_str;

    new_query_std += "&timestamp=" + getTimestamp();
    new_query_std += "&signature=" + getSignature(new_query_std);

    ADD_LOG("new_query_std = " << new_query_std);

    ExternalRequestSsl binance_request(m_url, m_port, api_path + "?" + new_query_std, method);
    binance_request.add_header("X-MBX-APIKEY", m_api_key);

    return response_handler(binance_request.send_request(""));
}

Json APIHandlerBinanceFutures::send_binance_normal_request(const std::string& api_path, const std::string& query_str, RequestMethod method)
{
    ExternalRequestSsl binance_request(m_url, m_port, api_path + (query_str != "" ? "?" + query_str : ""), method);

    return response_handler(binance_request.send_request(""));
}

Json APIHandlerBinanceFutures::response_handler(const std::string& response)
{
    std::size_t spliter = response.find("|");
    if (spliter == std::string::npos)
    {
        LOG(ERROR) << "APIHandlerBinance::response_handler, ERROR";
        return Json();
    }

    std::string header_str = response.substr(0, spliter);
    std::string body_str = response.substr(spliter+1);
  
    Json header = Json::parse(header_str);
    BinanceFutures::rate_limit_sync(header);

    return Json::parse(body_str);;
}

long long APIHandlerBinanceFutures::delta = 0;