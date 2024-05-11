#include <storage_source/coinbase/storage_source_coinbase.h>
#include <binance_utils.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_profit.h>

std::string StorageSourceCoinbase::init_info()
{
    std::string init_result = verify_valid_source();

    if (init_result == INIT_STORAGE_SOURCE_SUCCESS)
    {
        // Get user data source (streamming)
        get_user_data_stream();
    }

    return init_result;
}

std::string StorageSourceCoinbase::verify_valid_source()
{
    // Endpoint
    init_endpoint();

    // Get api_key + api_secret
    std::string init_result = init_api_key();

    return init_result;
}

std::string StorageSourceCoinbase::init_api_key()
{
    ADD_LOG("Get Api Key + Api Secret for Coinbase's user: " << m_user_id);
    ADD_LOG(m_user_id << "'s actiave source: " << get_db_name());

    Json user = MongoDB::instance()
        .set_db_and_collection(get_db_name(), "info")
        .find_one("user_id", m_user_id);

    if (user.is_null() == false)
    {
        m_api_key = (std::string&&)user["api_key"];
        m_api_secret = (std::string&&)user["api_secret"];
        m_passphrase = (std::string&&)user["passphrase"];

        ADD_LOG("Found api_key: " << m_api_key);
        ADD_LOG("Found api_secret: " << m_api_secret);
        ADD_LOG("Found passphrase: " << m_passphrase);

        std::string verify_result = verify_api_key();
        if (verify_result != VERIFY_API_KEY_AND_API_SECRET_SUCCESS)
        {
            return verify_result;
        }

        // Verify success
        ADD_LOG("Verify api_key + api_secret success !!!");
        return INIT_STORAGE_SOURCE_SUCCESS;
    }

    ADD_LOG(CANNOT_FIND_API_KEY);
    return CANNOT_FIND_API_KEY;
}

std::string StorageSourceCoinbase::verify_api_key()
{
    // First, send request to Binance to verify api_key + api_secret is valid or not
    Json account = APIHandlerCoinbase(nullptr)
        .set_authen_info(this)
        .send_coinbase_request("/accounts", "", "");

    if (account.has_field("message"))
    {
        ADD_LOG("Verify api_key + api_secret failed, msg: " << account["message"]);
        return account["message"];
    }

    // Then check if there is the same api key + api secret on the DB
    bsoncxx::v_noabi::document::view_or_value filter = document{} <<
        "api_key" << m_api_key <<
        "api_secret" << m_api_secret << finalize;

    Json binance_info = MongoDB::instance()
        .set_db_and_collection(get_db_name(), "info")
        .find_one(filter);

    if (binance_info.is_null() == false && (std::string&&)binance_info["user_id"] != m_user_id)
    {
        return API_KEY__USED_BY_ANOTHER_USER;
    }

    return VERIFY_API_KEY_AND_API_SECRET_SUCCESS;
}

void StorageSourceCoinbase::get_user_data_stream()
{
    // DataSourceBinanceUser::add_new_binance_user_data_stream(*this);
}

const std::string& StorageSourceCoinbase::get_api_key() const
{
    return m_api_key;
}

const std::string& StorageSourceCoinbase::get_api_secret() const
{
    return m_api_secret;
}

const std::string& StorageSourceCoinbase::get_passphrase() const
{
    return m_passphrase;
}