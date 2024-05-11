#include <utils.h>
#include <binance_utils.h>
#include <order_manager/order_manager.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_profit.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_current_price.h>

APIHandlerBinance24hProfit::APIHandlerBinance24hProfit(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinance24hProfit::get_filled_order_list_in_24h(StorageSource* storage_source)
{
    long today_0h = Utils::instance().get_0h_today_in_utc() * 1000;
    long tomorrow_0h = Utils::instance().get_0h_tomorrow_in_utc() * 1000;

    return get_filled_order_list_by_day(storage_source, today_0h, tomorrow_0h);
}

Json APIHandlerBinance24hProfit::get_filled_order_list_by_days_ago(StorageSource* storage_source, int days_ago)
{
    long days_before_0h = Utils::instance().get_0h_by_number_of_day_before_in_utc(days_ago) * 1000;
    long tomorrow_0h = Utils::instance().get_0h_tomorrow_in_utc() * 1000;

    return get_filled_order_list_by_day(storage_source, days_before_0h, tomorrow_0h);
}

Json APIHandlerBinance24hProfit::get_filled_order_list_by_day(StorageSource* storage_source, long from, long to)
{
    std::string user_id = storage_source->get_user_id();
    std::string db_name = storage_source->get_db_name();

    bsoncxx::v_noabi::document::view_or_value filter = document{} <<
        "user_id" << user_id <<
        "transactTime" << open_document <<
            "$gt" << from <<
            "$lte" << to <<
        close_document << finalize;

    Json execution_report_list = MongoDB::instance()
        .set_db_and_collection(db_name, "execution_report")
        .find_many(filter);

    Json res = Json::create_array();

    execution_report_list.for_each([&res, &user_id, &db_name](Json& execution_report)
    {
        std::string report_status = execution_report["status"];

        if (report_status == "CANCELLED") return;

        if (report_status == "NEW")
        {
            long order_id = execution_report["orderId"];
            Json order = OrderManager::instance().get_order(user_id, db_name, order_id);

            if (order.is_null() == false)
            {
                std::string status = order["status"];
                if (status != "NEW")
                {
                    return;
                }
            }
        }

        long order_id = execution_report["orderId"];
        Json order = OrderManager::instance().get_order(user_id, db_name, order_id);

        if (order.is_null() == false)
        {
            std::string status = order["status"];
            if (status == "CANCELLED" )
            {
                return;
            }
        }

        res.push_back(execution_report);
    });

    return res;
}

long double APIHandlerBinance24hProfit::get_asset_profit_by_order(Json& order)
{
    long double executedQty = std::stold((std::string&&)order["last_executed_quantity"]);

    std::string side = order["side"];
    if (side == "SELL")
    {
        executedQty *= -1;
    }

    return executedQty;
}

long double APIHandlerBinance24hProfit::get_currency_profit_by_order(Json& order)
{
    long double executedQty = std::stold((std::string&&)order["last_executed_quantity"]);
    long double price = std::stold((std::string&&)order["last_executed_price"]);

    long double currency_profit = executedQty * price;

    std::string side = order["side"];
    if (side == "BUY")
    {
        currency_profit *= -1;
    }

    return currency_profit;
}

long double APIHandlerBinance24hProfit::get_asset_commission_by_order(const std::string& asset, Json& order)
{
    if (asset == (std::string&&)order["commission_asset"])
    {
        return std::stold((std::string&&)order["commission_amount"]);
    }

    return 0.0f;
}

Json APIHandlerBinance24hProfit::calculate_profit_in_24h(const Json& orders, const std::vector<std::string>& asset_list)
{
    Json profit;
    APIHandlerBinanceCurrentPrice api_current_price(nullptr);
    long double res = 0;

    // Init profit asset to 0.0
    for (auto& asset: asset_list)
    {
        profit[asset] = (long double)0.0f;
    }

    for (auto& asset: asset_list)
    {
        orders.for_each([&asset, &profit](Json& order)
        {
            std::string symbol = order["symbol"];

            size_t pos = symbol.find(asset);
            if (pos != std::string::npos)
            {
                if (pos == 0) // asset is main
                {
                    profit[asset] += get_asset_profit_by_order(order);
                }
                else if (pos > 0 && (pos + asset.size()) == symbol.size()) // asset is currency
                {
                    profit[asset] += get_currency_profit_by_order(order);
                }

                profit[asset] -= get_asset_commission_by_order(asset, order);
            }
        });

        if (profit[asset] != (long double)0.0f)
        {
            Json asset_current_price = api_current_price.get_current_price_from_MongoDB_by_symbol_stream(asset + "USDT");

            if (asset_current_price.has_field("price"))
            {
                long double asset_price = asset_current_price["price"];
                long double asset_profit = profit[asset];

                // Asset profit in USDT
                profit["USDT"] += asset_profit * asset_price;
            }
        }

        // Remove all assets (unless USDT) as we only keep USDT in final profit / loss
        if (asset != "USDT")
        {
            profit.remove_field(asset);
        }
    }

    return profit;
}

HttpResponse APIHandlerBinance24hProfit::child_handle()
{
    Json order_list = get_filled_order_list_in_24h(m_user->get_active_storage_source().get());
    std::vector<std::string> asset_list = BinanceUtils::instance().get_asset_list(m_user->get_active_storage_source().get());

    Json response = {
        {"data", calculate_profit_in_24h(order_list, asset_list)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}