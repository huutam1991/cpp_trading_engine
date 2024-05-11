
#ifndef API_HANDLER_BINANCE_FUTURES_24H_PROFIT_H
#define API_HANDLER_BINANCE_FUTURES_24H_PROFIT_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

class APIHandlerBinanceFutures24hProfit : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFutures24hProfit(HttpRequest* request);

    static Json get_filled_order_list_in_24h(StorageSource* storage_source);
    static Json get_filled_order_list_by_days_ago(StorageSource* storage_source, int days_ago);
    static Json get_filled_order_list_by_day(StorageSource* storage_source, long from, long to);
    static Json calculate_profit_in_24h(const Json& orders, const std::vector<std::string>& asset_list);
    static long double get_asset_profit_by_order(Json& order);
    static long double get_currency_profit_by_order(Json& order);
    static long double get_asset_commission_by_order(const std::string& asset, Json& order);

private:
    virtual HttpResponse child_handle();

};

#endif //API_HANDLER_BINANCE_FUTURES_24H_PROFIT_H