#ifndef BINANCE_UTILS_H
#define BINANCE_UTILS_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <mongo_db/mongo_db.h>
//#include <api_handler/api_handler_binance_spot/api_handler_binance.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h> //MultiMarket

class BinanceUtils
{
    Singleton(BinanceUtils);

private:
    Json m_symbol_info_list;
    Json m_all_symbols_name;

    size_t get_rounded_number(const std::string& lot_size);
    Json get_exchange_info(const std::string& db_name);
    Json get_exchange_info_2(const std::string& db_name); //MultiMarket

    void init_all_symbols_in_real_net();
    void init_all_symbols_in_test_net();
    void init_all_symbols_in_binance_simulator();

    //MultiMarket
    std::string m_dbSourceName;
    std::string m_url;

public:
    void do_init();

    Json& get_symbol_info_with_exchange(const std::string& symbol);
    Json& get_symbol_info(const std::string& symbol);
    Json& get_symbol_info_futures(const std::string& symbol);

    Json  get_all_symbols_by_db_name(const std::string& db_name);

    Json get_24h_price_ticker(User* user);
    Json get_48h_price_ticker(User* user);
    Json get_price_ticker_by_date(User* user, long from, long to);
    int get_lot_size_by_symbol_test_net(const std::string& symbol);
    int get_lot_size_by_symbol_real_net(const std::string& symbol);
    int get_round_up_price_by_symbol_test_net(const std::string& symbol);
    int get_round_up_price_by_symbol_real_net(const std::string& symbol);
    long double get_current_price_by_symbol(const std::string& symbol);
    std::vector<std::string> get_asset_list(StorageSource* storage_source);
    std::vector<std::string> get_asset_list_from_Binance(StorageSource* storage_source);
    void fetch_new_asset_list(StorageSource* storage_source);
    void save_asset_list_to_DB(const std::vector<std::string>& asset_list, StorageSource* storage_source);

    //MultiMarket
    int get_lot_size_by_symbol_futures(const std::string& symbol);
    int get_round_up_price_by_symbol_futures(const std::string& symbol);
};

#endif //BINANCE_UTILS_H