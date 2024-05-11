#include <timer.h>
#include <external_request/external_request_ssl.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_user.h>

DataFeedBinanceBLVTUser::DataFeedBinanceBLVTUser(const string api_key, const string api_secret)
{
    m_api_key    = api_key;
    m_api_secret = api_secret;
}

DataFeedBinanceBLVTUser::~DataFeedBinanceBLVTUser()
{
}

void DataFeedBinanceBLVTUser::init()
{
}

void DataFeedBinanceBLVTUser::on_rest_response(Json& response)
{
    ADD_LOG("BinanceBLVTUser on_rest_response: " << response.get_string_value());
    for(auto cb : m_subscribed_list)
    {
        cb.second(response);
    }
}

size_t DataFeedBinanceBLVTUser::add_call_back(std::function<void(Json& payload)> call_back)
{
    std::unique_lock lock(m_bsuser_mutex);
    m_callback_id++;
    m_subscribed_list.insert({m_callback_id, call_back});

    ADD_LOG("add_call_back, " << m_callback_id);
    return m_callback_id;
}

void DataFeedBinanceBLVTUser::remove_call_back(size_t callback_id)
{
    std::unique_lock lock(m_bsuser_mutex);
    m_subscribed_list.erase(callback_id);
}

std::map<size_t, std::function<void(Json& payload)>> DataFeedBinanceBLVTUser::m_subscribed_list;