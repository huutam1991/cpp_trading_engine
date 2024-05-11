#ifndef BINANCE_ASSET_MANAGER_H
#define BINANCE_ASSET_MANAGER_H

#include <memory>
#include <unordered_map>
#include <mutex>

#include <util_macros.h>
 #include <app_constants.h>
#include <json/json.h>

using namespace std;

class BinanceAssetManager
{
    Singleton(BinanceAssetManager)

public:
    void init();
    void subscribe_data();
    void unsubscribe_data();
    
    Json get_trade_result(const long date_from, const long date_to);
    
    void query_blvt_record(string side, const long orderId, const long strategy_id);

    Json get_current_assets();
    bool check_asset_available(Market market, Json& query_json);

private:
    void process_events_updated(Json& data);

    void save_spot_and_future_trade(Json& trade);
    void save_trade_report_to_db(Json& trade_report);
    void send_trade_to_client(Json& trade);

    void fetch_positions();
    void save_position(Json& data, bool forward_to_client = true);
    void send_position_to_client(Json& position);

    mutex m_save_db_mutex;
    
    unordered_map<string, long double> m_baskets_map;
    unordered_map<string, long double> m_token_issued_map;
    unordered_map<string, Json> m_assets_map;

    size_t m_binance_spot_callback_id;
    size_t m_binance_futures_callback_id;
};

#endif //BINANCE_ASSET_MANAGER_H
