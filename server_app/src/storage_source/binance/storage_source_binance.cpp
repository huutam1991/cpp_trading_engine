#include <storage_source/binance/storage_source_binance.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance.h>
//#include <data_feed/data_feed_binance_spot/data_feed_binance_user.h>
//#include <binance_utils.h>
#include <exchanges/exchange_gateway.h>
//#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_profit.h>

std::string StorageSourceBinance::init_info()
{
    std::string init_result = verify_valid_source();

    if (init_result == INIT_STORAGE_SOURCE_SUCCESS)
    {
        // Get user data source (streamming)
        //get_user_data_stream();
        // if (m_user_id == "root")
        // {
        //     ExchangeGateWay::instance().start_user_feed(Market::BINANCE_SPOT, *this);
        // }
    }

    return init_result;
}

std::string StorageSourceBinance::verify_valid_source()
{
    // Endpoint
    //init_endpoint();

    // Get api_key + api_secret
    std::string init_result = init_api_key();

    return init_result;
}

std::string StorageSourceBinance::init_api_key()
{
    ADD_LOG("Get Api Key + Api Secret for Binance's user: " << m_user_id);
    ADD_LOG(m_user_id << "'s actiave source: " << get_db_name());

    Json user = MongoDB::instance()
        .set_db_and_collection(get_db_name(), "info")
        .find_one("user_id", m_user_id);

    if (user.is_null() == false)
    {
        m_api_key = (std::string&&)user["api_key"];
        m_api_secret = (std::string&&)user["api_secret"];

        ADD_LOG("Found api_key: " << m_api_key);
        ADD_LOG("Found api_secret: " << m_api_secret);

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

std::string StorageSourceBinance::verify_api_key()
{
    return VERIFY_API_KEY_AND_API_SECRET_SUCCESS;
    // First, send request to Binance to verify api_key + api_secret is valid or not
    Json account = APIHandlerBinance(nullptr)
        .set_authen_info(this)
        .send_binance_request("/api/v3/account", "recvWindow=50000");

    if (account.has_field("code") && (int)account["code"] < 0)
    {
        ADD_LOG("Verify api_key + api_secret failed, msg: " << account["msg"]);
        return account["msg"];
    }

    // Then check if there is the same api key + api secret on the DB
    bsoncxx::v_noabi::document::view_or_value filter = document{} <<
        "api_key" << m_api_key <<
        "api_secret" << m_api_secret << finalize;

    Json binance_info = MongoDB::instance()
        .set_db_and_collection(get_db_name(), "info")
        .find_one(filter);

    /*if (binance_info.is_null() == false && (std::string&&)binance_info["user_id"] != m_user_id)
    {
        return API_KEY__USED_BY_ANOTHER_USER;
    }*/

    return VERIFY_API_KEY_AND_API_SECRET_SUCCESS;
}

/*void StorageSourceBinance::get_user_data_stream()
{
    DataFeedBinanceUser::add_new_binance_user_data_stream(*this);
}*/

const std::string& StorageSourceBinance::get_api_key() const
{
    return m_api_key;
}

const std::string& StorageSourceBinance::get_api_secret() const
{
    return m_api_secret;
}