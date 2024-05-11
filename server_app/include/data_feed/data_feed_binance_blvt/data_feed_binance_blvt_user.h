#ifndef DATA_FEED_BINANCE_BLVT_USER_H
#define DATA_FEED_BINANCE_BLVT_USER_H

#include <map>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt.h>
#include <storage_source/binance/storage_source_binance.h>

using namespace std;

class DataFeedBinanceBLVTUser : public DataFeedBinanceBLVT
{
public:
    DataFeedBinanceBLVTUser(const string api_key, const string api_secret);
    DataFeedBinanceBLVTUser(){}
    ~DataFeedBinanceBLVTUser();
    
    virtual void init();
    void remove_call_back(size_t callback_id);
    size_t add_call_back(std::function<void(Json& payload)> call_back);
    void on_rest_response(Json& response);

protected:
    virtual bool standardize_data(const std::string&, Json&) {return false;}

private:
    std::string m_api_key;
    std::string m_api_secret;
    std::string m_listen_key;
    std::mutex  m_bsuser_mutex;

    size_t m_callback_id = 0;
    size_t m_schedule_task_id = 0;

    static std::map<size_t, std::function<void(Json& payload)>> m_subscribed_list;
};

#endif //DATA_FEED_BINANCE_BLVT_USER_H
