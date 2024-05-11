#include <binance_utils.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>
#include <storage_source/coinbase/storage_source_coinbase.h>
#include <jwt/jwt_manager.h>
#include <utils.h>

APIHandlerCoinbase::APIHandlerCoinbase(HttpRequest* request) : APIHandler(request)
{}

void APIHandlerCoinbase::synchronize_server_time()
{
    ExternalRequestSsl request("api.coinbase.com", "443", "/v2/time", RequestMethod::GET);
    Json data = Json::parse(request.send_request(""));

	long long ms_since_epoch = duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    long long server_time = data["data"]["epoch"];
    APIHandlerCoinbase::delta = server_time - ms_since_epoch;
}

std::string APIHandlerCoinbase::check_authentication()
{
    std::string check_valid_token = APIHandler::check_authentication();

    // Get m_url + m_api_key + m_api_secret
    if (check_valid_token == VALID_TOKEN)
    {
        set_authen_info(m_user->get_active_storage_source().get());
    }

    return check_valid_token;
}

APIHandlerCoinbase& APIHandlerCoinbase::set_authen_info(StorageSource* storage_source)
{
    //m_url = storage_source->get_url();

    if (storage_source->get_source_type() == SourceType::COINBASE_REALNET ||
        storage_source->get_source_type() == SourceType::COINBASE_TESTNET)
    {
        m_api_key = ((StorageSourceCoinbase*)storage_source)->get_api_key();
        m_api_secret = ((StorageSourceCoinbase*)storage_source)->get_api_secret();
        m_passphrase = ((StorageSourceCoinbase*)storage_source)->get_passphrase(); // TBD
    }

    ADD_LOG(">>> authen info - m_api_key = " << m_api_key);
    ADD_LOG(">>> authen info - m_api_secret = " << m_api_secret);
    ADD_LOG(">>> authen info - m_passphrase = " << m_passphrase);

    ADD_LOG(">>> authen info - source = " << storage_source->get_db_name());
    ADD_LOG(">>> authen info - url = " << m_url);
    ADD_LOG(">>> authen info - user_id = " << storage_source->get_user_id());

    return *this;
}

HttpResponse APIHandlerCoinbase::child_handle()
{
    return HttpResponse(ResponseStatusCode::OK_200, "APIHandlerCoinbase");
}

std::string APIHandlerCoinbase::getTimestamp()
{
	long long ms_since_epoch = duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return std::to_string(ms_since_epoch);
}

std::string APIHandlerCoinbase::encryptWithHMAC(const std::string& key, const char* data)
{
    unsigned char *result;
    int result_len = 32;
    std::string signature;

    std::vector<BYTE> key_decode = Utils::instance().base64_decode(key);
    result = HMAC(EVP_sha256(), &key_decode[0], key_decode.size(), const_cast<unsigned char *>(reinterpret_cast<const unsigned char*>(data)), strlen((char *)data), NULL, NULL);
    signature = Utils::instance().base64_encode((const BYTE*)result, result_len);

  	return signature;
}

std::string APIHandlerCoinbase::getSignature(const std::string& query)
{
	return encryptWithHMAC(m_api_secret.c_str(), query.c_str());
}

Json APIHandlerCoinbase::send_coinbase_request(const std::string& api_path, const std::string& query_str, const std::string& body, RequestMethod method)
{
    // m_api_key = "9JKeUsYQvk5jDgmW";
    // m_api_secret = "qNL5hBJQPVVkZSf1wnRqXA16xRHAQEu5";
    // m_api_key = "3f06bf3b787e8398a99c53532d17972f";
    // m_api_secret = "z4pePMQiFgfpvbZ7+QtrsKjvEzUlS3TGpkRn2Rph6dGk3CYbX3/iw1M6qLI6uzUWov0kufqyz2TMvfCiFsFl6w==";
    // m_passphrase = "123456";

    std::string route = api_path;
    if (query_str != "") route += "?" + query_str;

    std::string timestamp = getTimestamp();
    std::string message = timestamp + Utils::instance().get_request_method_string_by_id(method) + route + body;
    std::string signature = getSignature(message);

    ExternalRequestSsl request(m_url, "443", route, method);
    request.add_header("CB-ACCESS-SIGN", signature);
    request.add_header("CB-ACCESS-TIMESTAMP", timestamp);
    request.add_header("CB-ACCESS-KEY", m_api_key);
    request.add_header("CB-ACCESS-PASSPHRASE", m_passphrase);

    std::string response = request.send_request(body);

    return Json::parse(response);
}

Json APIHandlerCoinbase::send_coinbase_normal_request(const std::string& api_path, const std::string& query_str, RequestMethod method)
{
    ExternalRequestSsl request(m_url, "443", api_path + "?" + query_str, method);

    return Json::parse(request.send_request(""));
}

long long APIHandlerCoinbase::delta = 0;