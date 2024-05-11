#ifndef DATA_FEED_BINANCE_FUTURES_USER_H
#define DATA_FEED_BINANCE_FUTURES_USER_H

#include <map>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures.h>
#include <storage_source/binance_futures/storage_source_binance_futures.h>

using namespace std;

class DataFeedBinanceFuturesUser: public DataFeedBinanceFutures
{
public:
    DataFeedBinanceFuturesUser(const string api_key, const string api_secret);
    DataFeedBinanceFuturesUser() {}
    ~DataFeedBinanceFuturesUser();

    virtual void init();
    void remove_call_back(size_t callback_id);
    size_t add_call_back(std::function<void(Json& payload)> call_back);

private:
    std::string m_api_key;
    std::string m_api_secret;
    std::string m_listen_key;
    std::mutex  m_bfuser_mutex;

    size_t m_callback_id = 0;
    size_t m_schedule_task_id = 0;

    std::string get_listen_key();
    void add_timer_keep_alive_listen_key(size_t period);
    void del_timer_keep_alive_listen_key();
    
    void standardize_account_report(Json& report, Json& std_report);
    void standardize_execution_report(Json& report, Json& std_report);

    Json clone_standard_order_object(Json& json);
    Json clone_standard_execution_report(Json& json);

    static std::map<size_t, std::function<void(Json& payload)>> m_subscribed_list;
};

#endif //DATA_FEED_BINANCE_FUTURES_USER_H
